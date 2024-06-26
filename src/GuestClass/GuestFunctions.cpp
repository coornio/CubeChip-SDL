/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../Assistants/Well512.hpp"

#include "../HostClass/BasicVideoSpec.hpp"

#include "Guest.hpp"
#include "HexInput.hpp"
#include "ProgramControl.hpp"
#include "MemoryBanks.hpp"
#include "SoundCores.hpp"
#include "Registers.hpp"
#include "DisplayColors.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Guest                                                 */
/*------------------------------------------------------------------*/

VM_Guest::~VM_Guest() = default;
VM_Guest::VM_Guest(
	HomeDirManager* hdm_ptr,
	BasicVideoSpec* bvs_ptr,
	BasicAudioSpec* bas_ptr
)
	: HDM{ hdm_ptr }
	, BVS{ bvs_ptr }
	, BAS{ bas_ptr }
{
	Input   = std::make_unique<HexInput>();
	Wrand   = std::make_unique<Well512>();
	Mem     = std::make_unique<MemoryBanks>(this); // fine
	Program = std::make_unique<ProgramControl>(this, currFncSet); // needs Sound/Input
	Sound   = std::make_unique<SoundCores>(this, BAS); // needs Program
	Reg     = std::make_unique<Registers>(this, HDM); // fine
	Color   = std::make_unique<DisplayColors>();
}

bool VM_Guest::isSystemPaused() const { return _isSystemPaused && Program->ipf; }
bool VM_Guest::isDisplayReady() const { return _isDisplayReady; }

VM_Guest& VM_Guest::isSystemPaused(const bool state) { _isSystemPaused = state; return *this; }
VM_Guest& VM_Guest::isDisplayReady(const bool state) { _isDisplayReady = state; return *this; }

std::int32_t VM_Guest::fetchIPF()       const { return Program->ipf; }
double       VM_Guest::fetchFramerate() const { return Program->framerate; }

uint8_t& VM_Guest::mrw(const std::size_t idx) { return Mem->memory[idx & Program->limiter]; }
uint8_t& VM_Guest::VX()                       { return Reg->V[(Program->opcode >> 8) & 0xF]; }
uint32_t VM_Guest::NNNN()                     { return mrw(Program->counter) << 8 | mrw(Program->counter + 1); }

void VM_Guest::cycle() {
	Input->refresh();
	Program->handleTimersDec();
	Program->handleInterrupt();

	instructionLoop();

	Sound->renderAudio(BVS, Color.get(), Program.get());

	if (State.mega_enabled) { return; }

	renderToTexture(); // MegaChip calls it on demand
}

void VM_Guest::instructionLoop() {

	for (auto inst{ 0 }; inst < Program->ipf; ++inst) {
		auto HI = mrw(Program->counter++);
		auto LO = mrw(Program->counter++);
		Program->opcode = HI << 8 | LO;

		//std::cout << std::hex << "\n@ PC: 0x" << Program->counter - 2 << ", @ OP: 0x";
		//std::cout << std::setfill('0') << std::setw(4) << std::hex << Program->opcode;

		auto   X{ HI & 0xF };
		auto   Y{ LO >>  4 };
		auto   N{ LO & 0xF };
		auto NNN{ Program->opcode & 0xFFF };

		switch (HI >> 4) {
			case 0x0: switch (Program->opcode & 0x0FF0) {
				case 0x00B0:							// 00BN - scroll selected color plane N lines up *MEGACHIP*
				case 0x00D0:							// 00DN - scroll selected color plane N lines up *XOCHIP*
					if (!N) [[unlikely]] break;
					if (Quirk.waitScroll) [[unlikely]] {
						Program->setInterrupt(Interrupt::ONCE);
					}
					currFncSet->scrollUP(N);
					break;
				case 0x00C0:							// 00CN - scroll selected color plane N lines down *XOCHIP*
					if (!N) [[unlikely]] break;
					if (Quirk.waitScroll) [[unlikely]] {
						Program->setInterrupt(Interrupt::ONCE);
					}
					currFncSet->scrollDN(N);
					break;
				case 0x00E0: switch (N) {
					case 0x0:
						if (State.xochip_color) {		// 00E0 - erase selected color plane *XOCHIP*
							Mem->modifyViewport(BrushType::SUB);
						} else if (State.mega_enabled) {// 00E0 - push (and then clear) framebuffer to screen *MEGACHIP*
							Program->setInterrupt(Interrupt::ONCE);
							Mem->flushBuffers(FlushType::DISPLAY);
						} else {						// 00E0 - erase whole display
							Mem->modifyViewport(BrushType::CLR);
						}
						break;
					case 0x1:							// 00E1 - invert selected color plane *HWCHIP64*
						Mem->modifyViewport(BrushType::XOR);
						break;
					case 0xD:							// 00ED - stop signal *CHIP-8E*
						Program->setInterrupt(Interrupt::STOP);
						break;
					case 0xE:							// 00EE - return from subroutine
						Reg->routineReturn();
						break;
					[[unlikely]] default: Program->requestHalt();
				} break;
				case 0x00F0: switch (N) {
					case 0x0:							// 00F0 - return from subroutine *CHIP-8X MPD*
						Reg->routineReturn();
						break;
					case 0x1:							// 00F1 - set DRAW mode to ADD *HWCHIP64*
						Plane.brush = BrushType::ADD;
						break;
					case 0x2:							// 00F2 - set DRAW mode to SUB *HWCHIP64*
						Plane.brush = BrushType::SUB;
						break;
					case 0x3:							// 00F3 - set DRAW mode to XOR *HWCHIP64*
						Plane.brush = BrushType::XOR;
						break;
					case 0xB:							// 00FB - scroll selected color plane 4 pixels right *XOCHIP*
						if (Quirk.waitScroll) [[unlikely]] {
							Program->setInterrupt(Interrupt::ONCE);
						}
						currFncSet->scrollRT(4);
						break;
					case 0xC:							// 00FC - scroll selected color plane 4 pixels left *XOCHIP*
						if (Quirk.waitScroll) [[unlikely]] {
							Program->setInterrupt(Interrupt::ONCE);
						}
						currFncSet->scrollLT(4);
						break;
					case 0xD:							// 00FD - stop signal *SCHIP*
						Program->setInterrupt(Interrupt::STOP);
						break;
					case 0xE:							// 00FE - display == 64*32, erase the screen *XOCHIP*
						if (!State.mega_enabled) [[likely]] {
							setupDisplay(Resolution::LO, !State.schip_legacy);
						}
						break;
					case 0xF:							// 00FF - display == 128*64, erase the screen *XOCHIP*
						if (!State.mega_enabled) [[likely]] {
							setupDisplay(Resolution::HI, !State.schip_legacy);
						}
						break;
					[[unlikely]] default: Program->requestHalt();
				} break;
				default: {
					if (State.megachip_rom || State.gigachip_rom) {
						switch (X) {
							case 0x0: switch (LO) {
								case 0x10:				// 0010 - disable mega mode *MEGACHIP*
									State.mega_enabled = false;
									Quirk.waitScroll   = false;
									Quirk.idxRegNoInc  = false;
									Quirk.shiftVX      = false;
									Quirk.jmpRegX      = false;
									setupDisplay(Resolution::LO);
									Program->setFncSet(&SetClassic8);
									BVS->setTextureAlpha(0xFF);
									Sound->MC.reset(false);
									break;
								case 0x11:				// 0011 - enable mega mode *MEGACHIP*
									State.mega_enabled = true;
									Quirk.waitScroll   = true;
									Quirk.idxRegNoInc  = true;
									Quirk.shiftVX      = true;
									Quirk.jmpRegX      = true;
									setupDisplay(Resolution::MC);
									Program->setFncSet(&SetMegachip);
									Mem->flushBuffers(FlushType::DISCARD);
									BVS->setTextureAlpha(0xFF);
									Sound->MC.reset(true);
									break;
								[[unlikely]] default: Program->requestHalt();
							} break;
							case 0x1:					// 01NN - set I to NN'NNNN *MEGACHIP*
								Reg->I = (LO << 16) | NNNN();
								Program->counter += 2;
								break;
							case 0x2:					// 02NN - load NN palette colors from RAM at I *MEGACHIP*
								Mem->loadPalette(Reg->I, LO);
								break;
							case 0x3:					// 03NN - set sprite width to NN *MEGACHIP*
								Trait.W = LO ? LO : 256;
								break;
							case 0x4:					// 04NN - set sprite height to NN *MEGACHIP*
								Trait.H = LO ? LO : 256;
								break;
							case 0x5:					// 05NN - set screen brightness to NN *MEGACHIP*
								BVS->setTextureAlpha(LO);
								break;
							case 0x6:					// 060N - start digital sound from RAM at I, repeat if N == 0 *MEGACHIP*
								Sound->MC.enable(
									mrw(Reg->I + 0) <<  8 | mrw(Reg->I + 1),
									mrw(Reg->I + 2) << 16 | mrw(Reg->I + 3) << 8 | mrw(Reg->I + 4),
									Reg->I + 6, N == 0
								);
								break;
							case 0x7:					// 0700 - stop digital sound *MEGACHIP*
								Sound->MC.reset(false);
								break;
							case 0x8:					// 08YN - set trait flags to VY (Y > 0), blend mode to N *GIGACHIP*
								if (State.gigachip_rom) {
									Trait.setFlags(Reg->V[Y]);
									SetGigachip.chooseBlend(N);
								} else {				// 080N - set blend mode to N *MEGACHIP*
									static constexpr float alpha[]{ 1.0f, 0.25f, 0.50f, 0.75f };
									Trait.alpha = alpha[N > 3 ? 0 : N];
									SetMegachip.chooseBlend(N);
								}
								break;
							case 0x9:					// 09NN - set collision color to palette entry NN *MEGACHIP*
								Trait.collision = LO;
								break;
							[[unlikely]] default: Program->requestHalt();
						}
					}
					else switch (NNN) {
						case 0x151:						// 0151 - stop signal if delay timer == 0 *CHIP-8E*
							Program->setInterrupt(Interrupt::WAIT);
							break;
						case 0x188:						// 0188 - skip next instruction *CHIP-8E*
							Program->skipInstruction();
							break;
						case 0x216:						// 0216 - protect pages in V0 *CHIP-8 4PD*
							Reg->pageGuard = (3 - (Reg->V[0] - 1 & 0x3)) << 5;
							Reg->protectPages();
							break;
						case 0x200:						// 0200 - erase pages *CHIP-8 4PD*
						case 0x230:						// 0230 - erase pages *CHIP-8 2PD*
							Mem->clearPages(Reg->pageGuard);
							break;
						case 0x2A0:						// 02A0 - cycle background color *CHIP-8X*
						case 0x2F0:						// 02F0 - cycle background color *CHIP-8X MPD*
							Color->cycleBackground();
							break;
						[[unlikely]] default: Program->requestHalt();
					}
				}
			} break;
			case 0x1:									// 1NNN - jump to NNN; stop if PC == NNN (inf loop)
				Program->jumpInstruction(NNN);
				break;
			case 0x2:									// 2NNN - call subroutine
				Reg->routineCall(NNN);
				break;
			case 0x3:									// 3XNN - skip next instruction if VX == NN
				if (Reg->V[X] == LO) Program->skipInstruction();
				break;
			case 0x4:									// 4XNN - skip next instruction if VX != NN
				if (Reg->V[X] != LO) Program->skipInstruction();
				break;
			case 0x5: switch (N) {
				case 0x0:								// 5XY0 - skip next instruction if VX == VY
					if (Reg->V[X] == Reg->V[Y]) Program->skipInstruction();
					break;
				case 0x1:								// 5XY1 - skip next instruction if VX > VY *CHIP-8E*
					if (!State.chip8X_rom) {
						if (Reg->V[X] > Reg->V[Y]) Program->skipInstruction();
					} else {							// 5XY1 - add nibbles of VX,VY and modulo 8 to VX *CHIP-8X*
						const auto mask{ Program->screenLores ? 0x77 : 0xFF };
						const auto lenX{ (Reg->V[X] & 0xF0) + (Reg->V[Y] & 0xF0) };
						const auto lenY{ (Reg->V[X] + Reg->V[Y]) & 0xF };
						Reg->V[X] = static_cast<uint8_t>((lenX | lenY) & mask);
					}
					break;
				case 0x2: {								// 5XY2 - store range of registers to memory *CHIP-8E*
					if (State.chip8E_rom) [[unlikely]] {
						if (X < Y) for (auto Z{ X }; Z <= Y; ++Z)
							mrw(Reg->I++) = Reg->V[Z];
						else [[unlikely]]
							Program->setInterrupt(Interrupt::ONCE);
					} else {							// 5XY2 - store range of registers to memory *XOCHIP*
						const auto dist{ std::abs(X - Y) + 1 };
						if (X < Y) for (auto Z{ 0 }; Z < dist; ++Z)
							mrw(Reg->I + Z) = Reg->V[X + Z];
						else       for (auto Z{ 0 }; Z < dist; ++Z)
							mrw(Reg->I + Z) = Reg->V[X - Z];
					}
				} break;
				case 0x3: {								// 5XY3 - load range of registers from memory *CHIP-8E*
					if (State.chip8E_rom) [[unlikely]] {
						if (X < Y) for (auto Z{ X }; Z <= Y; ++Z)
							Reg->V[Z] = mrw(Reg->I++);
						else [[unlikely]]
							Program->setInterrupt(Interrupt::ONCE);
					} else {							// 5XY3 - load range of registers from memory *XOCHIP*
						const auto dist{ std::abs(X - Y) + 1 };
						if (X < Y) for (auto Z{ 0 }; Z < dist; ++Z)
							Reg->V[X + Z] = mrw(Reg->I + Z);
						else       for (auto Z{ 0 }; Z < dist; ++Z)
							Reg->V[X - Z] = mrw(Reg->I + Z);
					}
				} break;
				case 0x4: {								// 5XY4 - load range of colors from memory *EXPERIMENTAL*
					const auto dist{ std::abs(X - Y) + 1 };
					if (X < Y) for (auto Z{ 0 }; Z < dist; ++Z)
						Color->setBit332(X + Z, mrw(Reg->I + Z));
					else       for (auto Z{ 0 }; Z < dist; ++Z)
						Color->setBit332(X - Z, mrw(Reg->I + Z));
				} break;
				[[unlikely]] default: Program->requestHalt();
			} break;
			case 0x6:									// 6XNN - set VX = NN
				Reg->V[X] = LO;
				break;
			case 0x7:									// 7XNN - set VX = VX + NN
				Reg->V[X] += LO;
				break;
			case 0x8: switch (N) {
				case 0x0:								// 8XY0 - set VX = VY
					Reg->V[X] = Reg->V[Y];
					break;
				case 0x1:								// 8XY1 - set VX = VX | VY
					Reg->V[X] |= Reg->V[Y];
					if (Quirk.clearVF) Reg->V[0xF] = 0;
					break;
				case 0x2:								// 8XY2 - set VX = VX & VY
					Reg->V[X] &= Reg->V[Y];
					if (Quirk.clearVF) Reg->V[0xF] = 0;
					break;
				case 0x3:								// 8XY3 - set VX = VX ^ VY
					Reg->V[X] ^= Reg->V[Y];
					if (Quirk.clearVF) Reg->V[0xF] = 0;
					break;
				case 0x4: {								// 8XY4 - set VX = VX + VY, VF = carry
					const auto sum{ Reg->V[X] + Reg->V[Y] };
					Reg->V[X]   = static_cast<uint8_t>(sum);
					Reg->V[0xF] = static_cast<uint8_t>(sum >> 8);
				} break;
				case 0x5: {								// 8XY5 - set VX = VX - VY, VF = !borrow
					const bool borrow{ Reg->V[X] < Reg->V[Y] };
					Reg->V[X]   = Reg->V[X] - Reg->V[Y];
					Reg->V[0xF] = !borrow;
				} break;
				case 0x7: {								// 8XY7 - set VX = VY - VX, VF = !borrow
					const bool borrow{ Reg->V[Y] < Reg->V[X] };
					Reg->V[X]   = Reg->V[Y] - Reg->V[X];
					Reg->V[0xF] = !borrow;
				};  break;
				case 0x6: {								// 8XY6 - set VX = VY >> 1, VF = carry
					if (!Quirk.shiftVX) Reg->V[X] = Reg->V[Y];
					const bool lsb{ (Reg->V[X] & 1) == 1 };
					Reg->V[X]   = Reg->V[X] >> 1;
					Reg->V[0xF] = lsb;
				} break;
				case 0xE: {								// 8XYE - set VX = VY << 1, VF = carry
					if (!Quirk.shiftVX) Reg->V[X] = Reg->V[Y];
					const bool msb{ (Reg->V[X] >> 7) == 1 };
					Reg->V[X]   = Reg->V[X] << 1;
					Reg->V[0xF] = msb;
				} break;
				case 0xC: {								// 8XYC - set VX = VX * VY, VF = overflow *HWCHIP64*
					const auto mul{ Reg->V[X] * Reg->V[Y] };
					Reg->V[X]   = static_cast<uint8_t>(mul);
					Reg->V[0xF] = static_cast<uint8_t>(mul >> 8);
				} break;
				case 0xD: {								// 8XYD - set VX = VX / VY, VF = VX % VY *HWCHIP64*
					if (!Reg->V[Y]) {
						Reg->V[0xF] = Reg->V[X] = 0;
					} else {
						const auto remainder{ Reg->V[X] % Reg->V[Y] };
						Reg->V[X]   = Reg->V[X] / Reg->V[Y];
						Reg->V[0xF] = static_cast<uint8_t>(remainder);
					}
				} break;
				case 0xF: {								// 8XYF - set VX = VY / VX, VF = VX % VY *HWCHIP64*
					if (!Reg->V[X]) {
						Reg->V[0xF] = 0;
					} else {
						const auto remainder{ Reg->V[Y] % Reg->V[X] };
						Reg->V[X]   = Reg->V[Y] / Reg->V[X];
						Reg->V[0xF] = static_cast<uint8_t>(remainder);
					}
				} break;
				[[unlikely]] default: Program->requestHalt();
			} break;
			case 0x9: switch (N) {
				case 0x0:								// 9XY0 - skip next instruction if VX != VY
					if (Reg->V[X] != Reg->V[Y]) Program->skipInstruction();
					break;
				[[unlikely]] default: Program->requestHalt();
			} break;
			case 0xA:									// ANNN - set I = NNN
				Reg->I = NNN;
				break;
			case 0xB: {
				if (State.chip8E_rom) switch (X) {
					case 0xB:							// BBNN - jump to current PC + NN *CHIP-8E*
						Program->stepInstruction(-LO);
						break;
					case 0xF:							// BFNN - jump to current PC - NN *CHIP-8E*
						Program->stepInstruction(+LO);
						break;
					[[unlikely]] default: Program->requestHalt();
				}
				else if (State.chip8X_rom) {			// BXYN - set foreground color *CHIP-8X*
					if (N) {
						currFncSet->drawHiresColor(Reg->V[X], Reg->V[X + 1], Reg->V[Y] & 0x7, N);
						State.chip8X_hires = true;
					} else {
						currFncSet->drawLoresColor(Reg->V[X], Reg->V[X + 1], Reg->V[Y] & 0x7);
						State.chip8X_hires = false;
					}
				} else {								// BXNN - jump to NNN + V0 (else VX *SCHIP*)
					Program->jumpInstruction(NNN + (Quirk.jmpRegX ? Reg->V[X] : Reg->V[0]));
				}
			} break;
			case 0xC:									// CXNN - set VX = rnd(256) & NN
				Reg->V[X] = Wrand->get() & LO;
				break;
			case 0xD:									// DXYN - draw N sprite rows at VX and VY
				if (Quirk.waitVblank) [[unlikely]] {
					Program->setInterrupt(Interrupt::ONCE);
				}
				currFncSet->drawSprite(Reg->V[X], Reg->V[Y], N, Reg->I);
				break;
			case 0xE: switch (LO) {
				case 0x9E:								// EX9E - skip next instruction if key VX down (p1)
					if (Input->keyPressed(Reg->V[X], 0)) Program->skipInstruction();
					break;
				case 0xA1:								// EXA1 - skip next instruction if key VX up (p1)
					if (!Input->keyPressed(Reg->V[X], 0)) Program->skipInstruction();
					break;
				case 0xF2:								// EXF2 - skip next instruction if key VX down (p2) *CHIP-8X*
					if (Input->keyPressed(Reg->V[X], 16)) Program->skipInstruction();
					break;
				case 0xF5:								// EXF5 - skip next instruction if key VX up (p2) *CHIP-8X*
					if (!Input->keyPressed(Reg->V[X], 16)) Program->skipInstruction();
					break;
				[[unlikely]] default: Program->requestHalt();
			} break;
			case 0xF: switch (NNN) {
				case 0x000:								// F000 - set I = NEXT NNNN then skip instruction *XOCHIP*
					Reg->I = NNNN();
					Program->counter += 2;
					break;
				case 0x002:								// F002 - load audio pattern 0..15 from RAM at I..I+15 *XOCHIP*
					Sound->XO.loadPattern(Reg->I);
					break;
				case 0x100:								// F100 - long jump to NEXT NNNN *HWCHIP64*
					Program->counter = NNNN();
					break;
				case 0x200:								// F200 - call long subroutine *HWCHIP64*
					Reg->routineCall(NNNN());
					break;
				case 0x300:								// F300 - long jump to NEXT NNNN + V0 *HWCHIP64*
					Program->jumpInstruction(NNNN() + Reg->V[0]);
					break;
				default: switch (LO) {
					case 0x01:							// FX01 - set plane drawing to X *XOCHIP*
						Plane.selected = X;
						break;
					case 0x03:							// FX03 - load 24bit color X from RAM at I, I+1, I+2 *HWCHIP64*
						if (!State.chip8E_rom) {
							Color->bit[X] = 0xFF000000
								| mrw(Reg->I + 0) << 16
								| mrw(Reg->I + 1) << 8
								| mrw(Reg->I + 2);
						} else [[unlikely]] {			// FX03 - output VX to port 3 *CHIP-8E*
							Program->setInterrupt(Interrupt::ONCE);
						}
						break;
					case 0x07:							// FX07 - set VX = delay timer
						Reg->V[X] = Program->timerDelay;
						break;
					case 0x0A:							// FX0A - set VX = key, wait for keypress
						Sound->C8.setTone(Reg->SP, Program->counter);
						Program->setInterrupt(Interrupt::FX0A);
						if (State.mega_enabled) [[unlikely]] {
							Mem->flushBuffers(FlushType::DISPLAY);
						}
						break;
					case 0x15:							// FX15 - set delay timer = VX
						Program->timerDelay = Reg->V[X];
						break;
					case 0x18:							// FX18 - set sound timer = VX
						if (!State.chip8X_rom) [[likely]] {
							Sound->C8.setTone(Reg->SP, Program->counter);
						}
						Sound->beepFx0A = false;
						Program->timerSound = Reg->V[X] + (Reg->V[X] == 1);
						break;
					case 0x1B:							// FX1B - skip VX amount of bytes *CHIP-8E*
						Program->counter += Reg->V[X];
						break;
					case 0x1E:							// FX1E - set I = I + VX
						Reg->I += Reg->V[X];
						break;
					case 0x1F:							// FX1F - set I = I - VX *HWCHIP64*
						Reg->I -= Reg->V[X];
						break;
					case 0x29:							// FX29 - point I to 5 byte hex sprite from value in VX
						Reg->I = (Reg->V[X] & 0xF) * 5;
						break;
					case 0x30:							// FX30 - point I to 10 byte hex sprite from value in VX *SCHIP*
						Reg->I = (Reg->V[X] & 0xF) * 10 + 80;
						break;
					case 0x33:							// FX33 - store BCD of VX to RAM at I, I+1, I+2
						mrw(Reg->I + 0) = Reg->V[X] / 100;
						mrw(Reg->I + 1) = Reg->V[X] / 10 % 10;
						mrw(Reg->I + 2) = Reg->V[X] % 10;
						break;
					case 0x3A:							// FX3A - set sound pitch = VX *XOCHIP*
						Sound->XO.setPitch(Reg->V[X]);
						break;
					case 0x4F:							// FX4F - set delay timer = VX and wait *CHIP-8E*
						Program->setInterrupt(Interrupt::WAIT);
						Program->timerDelay = Reg->V[X];
						break;
					case 0x55:							// FX55 - store V0..VX to RAM at I..I+X
						for (auto idx{ 0 }; idx <= X; ++idx)
							mrw(Reg->I + idx) = Reg->V[idx];
						if (!Quirk.idxRegNoInc)
							Reg->I += X + !Quirk.idxRegMinus;
						break;
					case 0x65:							// FX65 - load V0..VX from RAM at I..I+X
						for (auto idx{ 0 }; idx <= X; ++idx)
							Reg->V[idx] = mrw(Reg->I + idx);
						if (!Quirk.idxRegNoInc)
							Reg->I += X + !Quirk.idxRegMinus;
						break;
					case 0x75:							// FX75 - store V0..VX to the P flags *XOCHIP*
						if (State.schip_legacy) [[unlikely]]
							X = std::min(X, 7);
						if (!Reg->writePermRegs(X + 1))
							Program->setInterrupt(Interrupt::STOP);
						break;
					case 0x85:							// FX85 - load V0..VX from the P flags *XOCHIP*
						if (State.schip_legacy) [[unlikely]]
							X = std::min(X, 7);
						if (!Reg->readPermRegs(X + 1))
							Program->setInterrupt(Interrupt::STOP);
						break;
					case 0xE3:							// FXE3 - wait for port 3 input, load into VX *CHIP-8E*
						Program->setInterrupt(Interrupt::ONCE);
						break;
					case 0xE7:							// FXE7 - read port 3 input, load to VX *CHIP-8E*
						Program->setInterrupt(Interrupt::ONCE);
						break;
					case 0xF8:							// FXF8 - output VX to port (sound freq) *CHIP-8X*
						Sound->C8.setTone(Reg->V[X]);
						break;
					case 0xFB:							// FXFB - wait for port input, load to VX *CHIP-8X*
						Program->setInterrupt(Interrupt::ONCE);
						break;
					[[unlikely]] default: Program->requestHalt();
				} break;
			} break;
		}
	}
}

/*------------------------------------------------------------------*/
/*  struct  VM_Guest::TextureTraits                                 */
/*------------------------------------------------------------------*/

void VM_Guest::TextureTraits::setFlags(const std::size_t bits) {
    rotate = bits >> 0 & 0x1; // false: as-is | true: 90° clockwise
    flip_X = bits >> 1 & 0x1; // flip on the X axis (rotation agnostic)
    flip_Y = bits >> 2 & 0x1; // flip on the Y axis (rotation agnostic)
    invert = bits >> 3 & 0x1; // invert RGB channels
    rgbmod = bits >> 4 & 0x7; // RGB channel swaps | sepia/grayscale
    nodraw = bits >> 7 * 0x1; // disable drawing, palette index only
    uneven = rotate && (W != H);
}
