/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <sstream>
#include <iomanip>
#include <utility>
#include <fstream>

#include "../Assistants/BasicLogger.hpp"
using namespace blogger;

#include "../HostClass/HomeDirManager.hpp"
#include "../HostClass/BasicVideoSpec.hpp"
#include "../HostClass/BasicAudioSpec.hpp"

#include "Guest.hpp"
#include "HexInput.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Guest                                                 */
/*------------------------------------------------------------------*/

VM_Guest::~VM_Guest() = default;
VM_Guest::VM_Guest(
	HomeDirManager& ref_HDM,
	BasicVideoSpec& ref_BVS,
	BasicAudioSpec& ref_BAS
)
	: HDM{ ref_HDM }
	, BVS{ ref_BVS }
	, BAS{ ref_BAS }
{
	_initBitColors();
	_initHexColors();
}

bool VM_Guest::isSystemPaused() const { return mSystemPaused || mCyclesPerFrame == 0; }
void VM_Guest::isSystemPaused(const bool state) { mSystemPaused = state; }

void VM_Guest::instructionLoop() {

	auto cycleCount{ 0 };
	for (; cycleCount < mCyclesPerFrame; ++cycleCount) {
		auto HI = readMemory(mProgCounter++);
		auto LO = readMemory(mProgCounter++);
		mInstruction = HI << 8 | LO;

		//std::cout << "  @ PC: 0x" << std::setfill('0') << std::setw(4) << std::hex << mProgCounter - 2;
		//std::cout << ", @ OP: 0x" << std::setfill('0') << std::setw(4) << std::hex << mInstruction << std::endl;

		const auto X{ HI & 0xF };
		const auto Y{ LO >>  4 };
		const auto N{ LO & 0xF };

		switch (HI >> 4) {
			case 0x0: switch (NN0()) {
				case 0x00B0:							// 00BN - scroll selected color plane N lines up *MEGACHIP*
				case 0x00D0:							// 00DN - scroll selected color plane N lines up *XOCHIP*
					if (Quirk.waitScroll) [[unlikely]]
						{ setInterrupt(Interrupt::FRAME); }
					if (!N) [[unlikely]] { break; }
					currFncSet->scrollUP(N);
					break;
				case 0x00C0:							// 00CN - scroll selected color plane N lines down *XOCHIP*
					if (Quirk.waitScroll) [[unlikely]]
						{ setInterrupt(Interrupt::FRAME); }
					if (!N) [[unlikely]] { break; }
					currFncSet->scrollDN(N);
					break;
				case 0x00E0: switch (N) {
					case 0x0:
						if (isManualRefresh()) {		// 00E0 - push (and then clear) framebuffer to screen *MEGACHIP*
							setInterrupt(Interrupt::FRAME);
							flushBuffers(FlushType::DISPLAY);
						} else {						// 00E0 - erase whole display (or plane *XO-CHIP*)
							if (Quirk.waitVblank) [[unlikely]]
								{ setInterrupt(Interrupt::FRAME); }
							if (isPixelBitColor()) {
								modifyDisplay_XO();
							} else {
								modifyDisplay_C8();
							}
						}
						break;
					case 0x1:							// 00E1 - invert selected color plane *HWCHIP64*
						modifyDisplay_HW();
						break;
					case 0xD:							// 00ED - stop signal *CHIP-8E*
						setInterrupt(Interrupt::SOUND);
						break;
					case 0xE:							// 00EE - return from subroutine
						if (routineReturn()) [[unlikely]]
							{ triggerError("Error :: Cannot return from empty stack!"); }
						break;
					[[unlikely]] default: triggerOpcodeError(mInstruction);
				} break;
				case 0x00F0: switch (N) {
					case 0x0:							// 00F0 - return from subroutine *CHIP-8X MPD*
						if (routineReturn()) [[unlikely]]
							{ triggerError("Error :: Cannot return from empty stack!"); }
						break;
					case 0x1:							// 00F1 - set DRAW mode to ADD *HWCHIP64*
						Trait.paintBrush = BrushType::ADD;
						break;
					case 0x2:							// 00F2 - set DRAW mode to SUB *HWCHIP64*
						Trait.paintBrush = BrushType::SUB;
						break;
					case 0x3:							// 00F3 - set DRAW mode to XOR *HWCHIP64*
						Trait.paintBrush = BrushType::XOR;
						break;
					case 0xB:							// 00FB - scroll selected color plane 4 pixels right *XOCHIP*
						if (Quirk.waitScroll) [[unlikely]]
							{ setInterrupt(Interrupt::FRAME); }
						currFncSet->scrollRT(4);
						break;
					case 0xC:							// 00FC - scroll selected color plane 4 pixels left *XOCHIP*
						if (Quirk.waitScroll) [[unlikely]]
							{ setInterrupt(Interrupt::FRAME); }
						currFncSet->scrollLT(4);
						break;
					case 0xD:							// 00FD - stop signal *SCHIP*
						setInterrupt(Interrupt::SOUND);
						break;
					case 0xE:							// 00FE - display == 64*32, erase the screen *XOCHIP*
						if (!isManualRefresh()) [[likely]] {
							prepDisplayArea(Resolution::LO, !State.schip_legacy);
						}
						break;
					case 0xF:							// 00FF - display == 128*64, erase the screen *XOCHIP*
						if (!isManualRefresh()) [[likely]] {
							prepDisplayArea(Resolution::HI, !State.schip_legacy);
						}
						break;
					[[unlikely]] default: triggerOpcodeError(mInstruction);
				} break;
				default: {
					if (State.megachip_rom || State.gigachip_rom) {
						switch (X) {
							case 0x0: switch (LO) {
								case 0x10:				// 0010 - disable mega mode *MEGACHIP*
									setInterrupt(Interrupt::FRAME);
									changeFunctionSet(&SetClassic8);

									isManualRefresh(false);
									resetAudioTrack();

									flushBuffers(FlushType::DISPLAY);
									prepDisplayArea(Resolution::LO);
									BVS.setTextureAlpha(0xFF);
									setBackgroundColorTo(BVS.getFrameColor());
									break;
								case 0x11:				// 0011 - enable mega mode *MEGACHIP*
									setInterrupt(Interrupt::FRAME);
									changeFunctionSet(&SetMegachip);

									isManualRefresh(true);
									resetAudioTrack();

									flushBuffers(FlushType::DISCARD);
									prepDisplayArea(Resolution::MC);
									BVS.setTextureAlpha(0xFF);
									setBackgroundColorTo(0);
									break;
								[[unlikely]] default: triggerOpcodeError(mInstruction);
							} break;
							case 0x1:					// 01NN - set I to NN'NNNN *MEGACHIP*
								mRegisterI = (LO << 16) | NNNN();
								mProgCounter += 2;
								break;
							case 0x2:					// 02NN - load NN palette colors from RAM at I *MEGACHIP*
								loadMegaPalette(LO);
								break;
							case 0x3:					// 03NN - set sprite width to NN *MEGACHIP*
								Texture.W = LO ? LO : 256;
								break;
							case 0x4:					// 04NN - set sprite height to NN *MEGACHIP*
								Texture.H = LO ? LO : 256;
								break;
							case 0x5:					// 05NN - set screen brightness to NN *MEGACHIP*
								BVS.setTextureAlpha(LO);
								break;
							case 0x6:					// 060N - start digital sound from RAM at I, repeat if N == 0 *MEGACHIP*
								startAudioTrack(N == 0);
								break;
							case 0x7:					// 0700 - stop digital sound *MEGACHIP*
								resetAudioTrack();
								break;
							case 0x8:					// 08YN - set trait flags to VY (Y > 0), blend mode to N *GIGACHIP*
								if (State.gigachip_rom) {
									setTextureFlags(mRegisterV[Y]);
									SetGigachip.chooseBlend(N);
								} else {				// 080N - set blend mode to N *MEGACHIP*
									static constexpr float alpha[]{ 1.0f, 0.25f, 0.50f, 0.75f };
									Texture.alpha = alpha[N > 3 ? 0 : N];
									SetMegachip.chooseBlend(N);
								}
								break;
							case 0x9:					// 09NN - set collision color to palette entry NN *MEGACHIP*
								Texture.collision = LO;
								break;
							[[unlikely]] default: triggerOpcodeError(mInstruction);
						}
					}
					else switch (NNN()) {
						case 0x151:						// 0151 - stop signal if delay timer == 0 *CHIP-8E*
							setInterrupt(Interrupt::DELAY);
							break;
						case 0x188:						// 0188 - skip next instruction *CHIP-8E*
							skipInstruction();
							break;
						case 0x216:						// 0216 - protect pages in V0 *CHIP-8 4PD*
							protectPages();
							break;
						case 0x200:						// 0200 - erase pages *CHIP-8 4PD*
						case 0x230:						// 0230 - erase pages *CHIP-8 2PD*
							clearPages();
							break;
						case 0x2A0:						// 02A0 - cycle background color *CHIP-8X*
						case 0x2F0:						// 02F0 - cycle background color *CHIP-8X MPD*
							cycleBackgroundColor();
							break;
						[[unlikely]] default: triggerOpcodeError(mInstruction);
					}
				}
			} break;
			case 0x1:									// 1NNN - jump to NNN; stop if PC == NNN (inf loop)
				if (jumpInstruction_C8(NNN())) [[unlikely]]
					{ setInterrupt(Interrupt::SOUND); }
				break;
			case 0x2:									// 2NNN - call subroutine
				if (routineCall(NNN())) [[unlikely]]
					{ triggerError("Error :: Cannot call with a full stack!"); }
				break;
			case 0x3:									// 3XNN - skip next instruction if VX == NN
				if (mRegisterV[X] == LO) { skipInstruction(); }
				break;
			case 0x4:									// 4XNN - skip next instruction if VX != NN
				if (mRegisterV[X] != LO) { skipInstruction(); }
				break;
			case 0x5: switch (N) {
				case 0x0:								// 5XY0 - skip next instruction if VX == VY
					if (mRegisterV[X] == mRegisterV[Y]) { skipInstruction(); }
					break;
				case 0x1:								// 5XY1 - skip next instruction if VX > VY *CHIP-8E*
					if (!State.chip8X_rom) {
						if (mRegisterV[X] > mRegisterV[Y]) { skipInstruction(); }
					} else {							// 5XY1 - add nibbles of VX,VY and modulo 8 to VX *CHIP-8X*
						const auto mask{ isLoresExtended() ? 0x77 : 0xFF};
						const auto lenX{ (mRegisterV[X] & 0xF0) + (mRegisterV[Y] & 0xF0) };
						const auto lenY{ (mRegisterV[X] + mRegisterV[Y]) & 0xF };
						mRegisterV[X] = static_cast<u8>((lenX | lenY) & mask);
					}
					break;
				case 0x2: {								// 5XY2 - store range of registers to memory *CHIP-8E*
					if (State.chip8E_rom) [[unlikely]] {
						if (X < Y) {
							for (auto Z{ X }; Z <= Y; ++Z) {
								writeMemory(mRegisterV[Z], mRegisterI++);
							}
						} else [[unlikely]]
							{ setInterrupt(Interrupt::FRAME); }
					} else {							// 5XY2 - store range of registers to memory *XOCHIP*
						const auto dist{ std::abs(X - Y) + 1 };
						if (X < Y) {
							for (auto Z{ 0 }; Z < dist; ++Z) {
								writeMemoryI(mRegisterV[X + Z], Z);
							}
						} else {
							for (auto Z{ 0 }; Z < dist; ++Z) {
								writeMemoryI(mRegisterV[X - Z], Z);
							}
						}
					}
				} break;
				case 0x3: {								// 5XY3 - load range of registers from memory *CHIP-8E*
					if (State.chip8E_rom) [[unlikely]] {
						if (X < Y) {
							for (auto Z{ X }; Z <= Y; ++Z) {
								mRegisterV[Z] = readMemory(mRegisterI++);
							}
						} else [[unlikely]]
							{ setInterrupt(Interrupt::FRAME); }
					} else {							// 5XY3 - load range of registers from memory *XOCHIP*
						const auto dist{ std::abs(X - Y) + 1 };
						if (X < Y) {
							for (auto Z{ 0 }; Z < dist; ++Z) {
								mRegisterV[X + Z] = readMemoryI(Z);
							}
						} else {
							for (auto Z{ 0 }; Z < dist; ++Z) {
								mRegisterV[X - Z] = readMemoryI(Z);
							}
						}
					}
				} break;
				case 0x4: {								// 5XY4 - load range of colors from memory *EXPERIMENTAL*
					const auto dist{ std::abs(X - Y) + 1 };
					if (X < Y) {
						for (auto Z{ 0 }; Z < dist; ++Z) {
							setColorBit332(X + Z, readMemoryI(Z));
						}
					} else {
						for (auto Z{ 0 }; Z < dist; ++Z) {
							setColorBit332(X - Z, readMemoryI(Z));
						}
					}
				} break;
				[[unlikely]] default: triggerOpcodeError(mInstruction);
			} break;
			case 0x6:									// 6XNN - set VX = NN
				mRegisterV[X]  = LO;
				break;
			case 0x7:									// 7XNN - set VX = VX + NN
				mRegisterV[X] += LO;
				break;
			case 0x8: switch (N) {
				case 0x0:								// 8XY0 - set VX = VY
					mRegisterV[X]  = mRegisterV[Y];
					break;
				case 0x1:								// 8XY1 - set VX = VX | VY
					mRegisterV[X] |= mRegisterV[Y];
					if (Quirk.clearVF) { mRegisterV[0xF] = 0; }
					break;
				case 0x2:								// 8XY2 - set VX = VX & VY
					mRegisterV[X] &= mRegisterV[Y];
					if (Quirk.clearVF) { mRegisterV[0xF] = 0; }
					break;
				case 0x3:								// 8XY3 - set VX = VX ^ VY
					mRegisterV[X] ^= mRegisterV[Y];
					if (Quirk.clearVF) { mRegisterV[0xF] = 0; }
					break;
				case 0x4: {								// 8XY4 - set VX = VX + VY, VF = carry
					const auto sum{ mRegisterV[X] + mRegisterV[Y] };
					mRegisterV[X]   = static_cast<u8>(sum);
					mRegisterV[0xF] = static_cast<u8>(sum >> 8);
				} break;
				case 0x5: {								// 8XY5 - set VX = VX - VY, VF = !borrow
					const bool borrow{ mRegisterV[X] < mRegisterV[Y] };
					mRegisterV[X]   = mRegisterV[X] - mRegisterV[Y];
					mRegisterV[0xF] = !borrow;
				} break;
				case 0x7: {								// 8XY7 - set VX = VY - VX, VF = !borrow
					const bool borrow{ mRegisterV[Y] < mRegisterV[X] };
					mRegisterV[X]   = mRegisterV[Y] - mRegisterV[X];
					mRegisterV[0xF] = !borrow;
				};  break;
				case 0x6: {								// 8XY6 - set VX = VY >> 1, VF = carry
					if (!Quirk.shiftVX) mRegisterV[X] = mRegisterV[Y];
					const bool lsb{ (mRegisterV[X] & 1) == 1 };
					mRegisterV[X]   = mRegisterV[X] >> 1;
					mRegisterV[0xF] = lsb;
				} break;
				case 0xE: {								// 8XYE - set VX = VY << 1, VF = carry
					if (!Quirk.shiftVX) mRegisterV[X] = mRegisterV[Y];
					const bool msb{ (mRegisterV[X] >> 7) == 1 };
					mRegisterV[X]   = mRegisterV[X] << 1;
					mRegisterV[0xF] = msb;
				} break;
				case 0xC: {								// 8XYC - set VX = VX * VY, VF = overflow *HWCHIP64*
					const auto mul{ mRegisterV[X] * mRegisterV[Y] };
					mRegisterV[X]   = static_cast<u8>(mul);
					mRegisterV[0xF] = static_cast<u8>(mul >> 8);
				} break;
				case 0xD: {								// 8XYD - set VX = VX / VY, VF = VX % VY *HWCHIP64*
					if (!mRegisterV[Y]) {
						mRegisterV[0xF] = mRegisterV[X] = 0;
					} else {
						const auto remainder{ mRegisterV[X] % mRegisterV[Y] };
						mRegisterV[X]   = mRegisterV[X] / mRegisterV[Y];
						mRegisterV[0xF] = static_cast<u8>(remainder);
					}
				} break;
				case 0xF: {								// 8XYF - set VX = VY / VX, VF = VX % VY *HWCHIP64*
					if (!mRegisterV[X]) {
						mRegisterV[0xF] = 0;
					} else {
						const auto remainder{ mRegisterV[Y] % mRegisterV[X] };
						mRegisterV[X]   = mRegisterV[Y] / mRegisterV[X];
						mRegisterV[0xF] = static_cast<u8>(remainder);
					}
				} break;
				[[unlikely]] default: triggerOpcodeError(mInstruction);
			} break;
			case 0x9: switch (N) {
				case 0x0:								// 9XY0 - skip next instruction if VX != VY
					if (mRegisterV[X] != mRegisterV[Y]) skipInstruction();
					break;
				[[unlikely]] default: triggerOpcodeError(mInstruction);
			} break;
			case 0xA:									// ANNN - set I = NNN
				mRegisterI = NNN();
				break;
			case 0xB: {
				if (State.chip8E_rom) switch (X) {
					case 0xB:							// BBNN - jump to current PC - NN *CHIP-8E*
						if (jumpInstruction_8E(-LO)) [[unlikely]]
							{ setInterrupt(Interrupt::SOUND); }
						break;
					case 0xF:							// BFNN - jump to current PC + NN *CHIP-8E*
						if (jumpInstruction_8E(+LO)) [[unlikely]]
							{ setInterrupt(Interrupt::SOUND); }
						break;
					[[unlikely]] default: triggerOpcodeError(mInstruction);
				}
				else if (State.chip8X_rom) {			// BXYN - set foreground color *CHIP-8X*
					if (N) {
						currFncSet->drawHiresColor(
							mRegisterV[X], mRegisterV[X + 1], mRegisterV[Y] & 0x7, N
						);
					} else {
						currFncSet->drawLoresColor(
							mRegisterV[X], mRegisterV[X + 1], mRegisterV[Y] & 0x7
						);
					}
				} else {								// BXNN - jump to NNN + V0 (else VX *SCHIP*)
					const auto addr{ NNN() + (Quirk.jmpRegX ? mRegisterV[X] : mRegisterV[0])};
					if (jumpInstruction_C8(addr)) [[unlikely]]
						{ setInterrupt(Interrupt::SOUND); }
				}
			} break;
			case 0xC:									// CXNN - set VX = rnd(256) & NN
				mRegisterV[X] = Wrand.get() & LO;
				break;
			case 0xD:									// DXYN - draw N sprite rows at VX and VY
				if (Quirk.waitVblank) [[unlikely]]
					{ setInterrupt(Interrupt::FRAME); }
				currFncSet->drawSprite(X, Y, N);
				break;
			case 0xE: switch (LO) {
				case 0x9E:								// EX9E - skip next instruction if key VX down (p1)
					if ( Input.keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
					break;
				case 0xA1:								// EXA1 - skip next instruction if key VX up (p1)
					if (!Input.keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
					break;
				case 0xF2:								// EXF2 - skip next instruction if key VX down (p2) *CHIP-8X*
					if ( Input.keyHeld_P2(mRegisterV[X])) { skipInstruction(); }
					break;
				case 0xF5:								// EXF5 - skip next instruction if key VX up (p2) *CHIP-8X*
					if (!Input.keyHeld_P2(mRegisterV[X])) { skipInstruction(); }
					break;
				[[unlikely]] default: triggerOpcodeError(mInstruction);
			} break;
			case 0xF: switch (NNN()) {
				case 0x000:								// F000 - set I = NEXT NNNN then skip instruction *XOCHIP*
					mRegisterI = NNNN();
					mProgCounter += 2;
					break;
				case 0x002:								// F002 - load audio pattern 0..15 from RAM at I..I+15 *XOCHIP*
					fetchPattern_XO();
					break;
				case 0x100:								// F100 - long jump to NEXT NNNN *HWCHIP64*
					mProgCounter = NNNN();
					break;
				case 0x200:								// F200 - call long subroutine *HWCHIP64*
					if (routineCall(NNNN())) [[unlikely]]
						{ triggerError("Error :: Cannot call with a full stack!"); }
					break;
				case 0x300:								// F300 - long jump to NEXT NNNN + V0 *HWCHIP64*
					if (jumpInstruction_C8(NNNN() + mRegisterV[0])) [[unlikely]]
						{ setInterrupt(Interrupt::SOUND); }
					break;
				default: switch (LO) {
					case 0x01:							// FX01 - set plane drawing to X *XOCHIP*
						Trait.maskPlane = X;
						break;
					case 0x03:							// FX03 - load 24bit color X from RAM at I, I+1, I+2 *HWCHIP64*
						if (!State.chip8E_rom) {
							Color.bit[X] = 0xFF000000
								| readMemoryI(0) << 16
								| readMemoryI(1) <<  8
								| readMemoryI(2);
						} else [[unlikely]] {			// FX03 - output VX to port 3 *CHIP-8E*
							setInterrupt(Interrupt::FRAME);
						}
						break;
					case 0x07:							// FX07 - set VX = delay timer
						mRegisterV[X] = mDelayTimer;
						break;
					case 0x0A:							// FX0A - set VX = key, wait for keypress
						setInterrupt(Interrupt::INPUT);
						if (isManualRefresh()) [[unlikely]] {
							flushBuffers(FlushType::DISPLAY);
						}
						break;
					case 0x15:							// FX15 - set delay timer = VX
						mDelayTimer = mRegisterV[X];
						break;
					case 0x18:							// FX18 - set sound timer = VX
						if (!State.chip8X_rom) [[likely]]
							{ setAudioTone_C8(); }
						mBuzzLight = false;
						mSoundTimer = mRegisterV[X] + (mRegisterV[X] == 1);
						break;
					case 0x1B:							// FX1B - skip VX amount of bytes *CHIP-8E*
						mProgCounter += mRegisterV[X];
						break;
					case 0x1E:							// FX1E - set I = I + VX
						mRegisterI += mRegisterV[X];
						break;
					case 0x1F:							// FX1F - set I = I - VX *HWCHIP64*
						mRegisterI -= mRegisterV[X];
						break;
					case 0x29:							// FX29 - point I to 5 byte hex sprite from value in VX
						mRegisterI = (mRegisterV[X] & 0xF) * 5;
						break;
					case 0x30:							// FX30 - point I to 10 byte hex sprite from value in VX *SCHIP*
						mRegisterI = (mRegisterV[X] & 0xF) * 10 + 80;
						break;
					case 0x33:							// FX33 - store BCD of VX to RAM at I, I+1, I+2
						writeMemoryI(mRegisterV[X] / 100,     0);
						writeMemoryI(mRegisterV[X] / 10 % 10, 1);
						writeMemoryI(mRegisterV[X] % 10,      2);
						break;
					case 0x3A:							// FX3A - set sound pitch = VX *XOCHIP*
						setAudioTone_XO(mRegisterV[X]);
						break;
					case 0x4F:							// FX4F - set delay timer = VX and wait *CHIP-8E*
						setInterrupt(Interrupt::DELAY);
						mDelayTimer = mRegisterV[X];
						break;
					case 0x55:							// FX55 - store V0..VX to RAM at I..I+X
						for (auto idx{ 0 }; idx <= X; ++idx)
							{ writeMemoryI(mRegisterV[idx], idx); }
						if (!Quirk.idxRegNoInc) [[likely]]
							{ mRegisterI += X + !Quirk.idxRegMinus; }
						break;
					case 0x65:							// FX65 - load V0..VX from RAM at I..I+X
						for (auto idx{ 0 }; idx <= X; ++idx)
							{ mRegisterV[idx] = readMemoryI(idx); }
						if (!Quirk.idxRegNoInc) [[likely]]
							{ mRegisterI += X + !Quirk.idxRegMinus; }
						break;
					case 0x75:							// FX75 - store V0..VX to the P flags *XOCHIP*
						if (writePermRegs((State.schip_legacy ? std::min(X, 7) : X) + 1)) [[unlikely]]
							{ triggerError("Error :: Failed writing persistent registers!"); }
						break;
					case 0x85:							// FX85 - load V0..VX from the P flags *XOCHIP*
						if (readPermRegs((State.schip_legacy ? std::min(X, 7) : X) + 1)) [[unlikely]]
							{ triggerError("Error :: Failed reading persistent registers!"); }
						break;
					case 0xE3:							// FXE3 - wait for port 3 input, load into VX *CHIP-8E*
						setInterrupt(Interrupt::FRAME);
						break;
					case 0xE7:							// FXE7 - read port 3 input, load to VX *CHIP-8E*
						setInterrupt(Interrupt::FRAME);
						break;
					case 0xF8:							// FXF8 - output VX to port (sound freq) *CHIP-8X*
						setAudioTone_8X(mRegisterV[X]);
						break;
					case 0xFB:							// FXFB - wait for port input, load to VX *CHIP-8X*
						setInterrupt(Interrupt::FRAME);
						break;
					[[unlikely]] default: triggerOpcodeError(mInstruction);
				} break;
			} break;
		}
	}
	mTotalCycles += cycleCount;
}

std::string VM_Guest::hexOpcode(const u32 opcode) const {
	std::stringstream out;
	out << std::setfill('0') << std::setw(4)
		<< std::uppercase    << std::hex
		<< opcode;
	return out.str();
}

void VM_Guest::initProgramParams(const u32 counter, const s32 cpf) noexcept {
	mProgCounter    = counter;
	mCyclesPerFrame = cpf;
	mFramerate      = 60.0;
	mInterruptType  = Interrupt::CLEAR;
}

void VM_Guest::calculateBoostCPF(const s32 cpf) noexcept {
	if (cpf) { mCyclesPerFrame = cpf; }
	boost = (mCyclesPerFrame < 50) ? (mCyclesPerFrame >> 1) : 0;
}

void VM_Guest::changeFunctionSet(FncSetInterface* _fncSet) noexcept {
	currFncSet = _fncSet;
}

void VM_Guest::setInterrupt(const Interrupt type) {
	mInterruptType  = type;
	mCyclesPerFrame = -std::abs(mCyclesPerFrame);
}

void VM_Guest::triggerError(std::string_view msg) {
	blog.stdLogOut(msg.data());
	setInterrupt(Interrupt::ERROR);
}

void VM_Guest::triggerOpcodeError(const u32 opcode) {
	if (opcode & 0xF000) {
		blog.stdLogOut("Error :: Unknown instruction detected: " + hexOpcode(opcode));
	} else {
		blog.stdLogOut("Error :: ML routines are unsupported: " + hexOpcode(opcode));
	}
	setInterrupt(Interrupt::ERROR);
}

void VM_Guest::processFrame() {
	if (isSystemPaused()) { return; }
	else { ++mTotalFrames; }

	Input.updateKeyStates();

	if ( mDelayTimer) { --mDelayTimer; }
	if ( mSoundTimer) { --mSoundTimer; }
	if (!mSoundTimer) { mBuzzLight = false; }

	handleFrameWait();

	instructionLoop();

	handleInputWait();

	renderAudioData();

	if (!isManualRefresh()) {
		renderToTexture();
	}
}

void VM_Guest::handleFrameWait() noexcept {
	switch (mInterruptType) {

		case Interrupt::FRAME: // resumes emulation after a single frame pause
			mCyclesPerFrame = std::abs(mCyclesPerFrame);
			return;

		case Interrupt::SOUND: // stops emulation when sound timer reaches 0
			if (!mSoundTimer) {
				mInterruptType  = Interrupt::FINAL;
				mCyclesPerFrame = 0;
			}
			return;

		case Interrupt::DELAY: // pauses emulation while delay timer is not 0
			if (!mDelayTimer) {
				mInterruptType  = Interrupt::CLEAR;
				mCyclesPerFrame = std::abs(mCyclesPerFrame);
			}
			return;
	}
}

void VM_Guest::handleInputWait() noexcept {
	switch (mInterruptType) {

		case Interrupt::INPUT: // resumes emulation when key press event for Fx0A
			if (Input.keyPressed(VX(), mTotalFrames)) {
				mInterruptType = Interrupt::CLEAR;
				mCyclesPerFrame = std::abs(mCyclesPerFrame);
				mSoundTimer = 2;
				mBuzzLight  = true;
				setAudioTone_C8();
			}
			return;

		case Interrupt::FINAL:
		case Interrupt::ERROR:
			mCyclesPerFrame = 0;
			return;
	}
}

void VM_Guest::modifyDisplay_C8() {
	displayBuffer[0].wipeAll();
}

void VM_Guest::modifyDisplay_XO() {
	for (auto P{ 0 }; P < 4; ++P) {
		if (!(Trait.maskPlane & (1 << P))) { continue; }
		displayBuffer[P].wipeAll();
	}
}

void VM_Guest::modifyDisplay_HW() {
	for (auto P{ 0 }; P < 4; ++P) {
		if (!(Trait.maskPlane & (1 << P))) { continue; }
		for (auto& px : displayBuffer[P].span()) { px ^= 1; }
	}
}

void VM_Guest::flushBuffers(const FlushType option) {
	switch (option) {
		case FlushType::DISCARD:
			for (auto& elem : megaPalette) { elem = {}; }
			backgroundBuffer.wipeAll();
			collisionPalette.wipeAll();
			break;

		case FlushType::DISPLAY:
			foregroundBuffer = backgroundBuffer;
			backgroundBuffer.wipeAll();
			collisionPalette.wipeAll();
			renderToTexture();
			break;
	}
}

void VM_Guest::loadMegaPalette(const s32 count) noexcept {
	auto index{ mRegisterI };
	for (auto pos{ 0 }; pos < count; index += 4) {
		megaPalette[++pos] = readMemory(index + 0) << 24
						   | readMemory(index + 1) << 16
						   | readMemory(index + 2) <<  8
						   | readMemory(index + 3);
	}
}

void VM_Guest::clearPages() {
	auto row{ Trait.pageGuard };
	while (row < Trait.H) {
		displayBuffer[0][row++].wipeAll();
	}
}

void VM_Guest::setBackgroundColorTo(const u32 color) const noexcept {
	BVS.setFrameColor(color);
}

void VM_Guest::cycleBackgroundColor() noexcept {
	BVS.setFrameColor(Color.BackColors[Color.bgindex++ & 0x3]);
}

bool VM_Guest::routineCall(const u32 addr) noexcept {
	mStackBank[mStackTop++ & 0xF] = mProgCounter;
	mProgCounter = addr;
	return false;
	//if (mStackTop < mStackBank + 16) {
	//	*mStackTop++ = mProgCounter;
	//	mProgCounter = addr;
	//	return false;
	//} else { return true; }
}

bool VM_Guest::routineReturn() noexcept {
	mProgCounter = mStackBank[--mStackTop & 0xF];
	return false;
	//if (mStackTop > mStackBank) {
	//	mProgCounter = *(--mStackTop);
	//	return false;
	//} else { return true; }
}

void VM_Guest::protectPages() noexcept {
	Trait.pageGuard = (3 - (mRegisterV[0] - 1 & 0x3)) << 5;
}

bool VM_Guest::readPermRegs(const usz X) {
	const auto path{ HDM.permRegs / HDM.sha1 };

	if (std::filesystem::exists(path)) {
		if (!std::filesystem::is_regular_file(path)) {
			blog.stdLogOut("SHA1 file is malformed: " + path.string());
			return true;
		}

		std::ifstream in(path, std::ios::binary);
		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			const auto totalBytes{ static_cast<usz>(in.tellg()) };
			in.seekg(0, std::ios::beg);

			in.read(reinterpret_cast<char*>(mRegisterV), std::min(totalBytes, X));
			in.close();

			if (totalBytes < X) {
				std::fill_n(mRegisterV + totalBytes, X - totalBytes, u8());
			}
		} else {
			blog.stdLogOut("Could not open SHA1 file to read: " + path.string());
			return true;
		}
	} else {
		std::fill_n(mRegisterV, X, u8());
	}
	return false;
}

bool VM_Guest::writePermRegs(const usz X) {
	const auto path{ HDM.permRegs / HDM.sha1 };

	if (std::filesystem::exists(path)) {
		if (!std::filesystem::is_regular_file(path)) {
			blog.stdLogOut("SHA1 file is malformed: " + path.string());
			return true;
		}

		char tempV[16]{};
		std::ifstream in(path, std::ios::binary);

		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			const auto totalBytes{ in.tellg() };
			in.seekg(0, std::ios::beg);

			in.read(tempV, std::min<std::streamsize>(totalBytes, X));
			in.close();
		} else {
			blog.stdLogOut("Could not open SHA1 file to read: " + path.string());
			return true;
		}

		std::copy_n(mRegisterV, X, tempV);

		std::ofstream out(path, std::ios::binary);
		if (out.is_open()) {
			out.write(tempV, 16);
			out.close();
		} else {
			blog.stdLogOut("Could not open SHA1 file to write: " + path.string());
			return true;
		}
	} else {
		std::ofstream out(path, std::ios::binary);
		if (out.is_open()) {
			out.write(reinterpret_cast<const char*>(mRegisterV), X);
			if (X < 16) {
				const char padding[16]{};
				out.write(padding, 16 - X);
			}
			out.close();
		} else {
			blog.stdLogOut("Could not open SHA1 file to write: " + path.string());
			return true;
		}
	}
	return false;
}

void VM_Guest::skipInstruction() noexcept {
	switch (readMemory(mProgCounter)) {
		case 0x01:
			mProgCounter += 4;
			break;

		case 0xF0: case 0xF1:
		case 0xF2: case 0xF3:
			mProgCounter += (readMemory(mProgCounter + 1)) ? 2 : 4;
			break;

		default:
			mProgCounter += 2;
	}
}

void VM_Guest::skipInstruction_C8() noexcept {
	mProgCounter += 2;
}

void VM_Guest::skipInstruction_MC() noexcept {
	mProgCounter += readMemory(mProgCounter) == 0x1 ? 4 : 2;
}

void VM_Guest::skipInstruction_XO() noexcept {
	mProgCounter += NNNN() == 0xF000 ? 4 : 2;
}

void VM_Guest::skipInstruction_HW() noexcept {
	mProgCounter += 2;
	switch (NNNN()) {
		case 0xF000: case 0xF100:
		case 0xF200: case 0xF300:
			mProgCounter += 4;
	}
}

bool VM_Guest::jumpInstruction_C8(const u32 next) noexcept {
	if (mProgCounter - 2 != next) [[likely]] {
		mProgCounter = next;
		return false;
	} else { return true; }
}

bool VM_Guest::jumpInstruction_8E(const s32 step) noexcept {
	if (step) [[likely]] {
		mProgCounter += step - 2;
		return false;
	} else { return true; }
}


/*==================================================================*/
	#pragma region AUDIO GENERATION
/*==================================================================*/

void VM_Guest::setAudioTone_C8() noexcept {
	C8.mTone = (160.0f + 8.0f * (
		(mProgCounter >> 1) + peekStackHead() + 1 & 0x3E)
	) / BAS.getFrequency();
}

void VM_Guest::setAudioTone_8X(const u8 pitch) noexcept {
	C8.mTone = (160.0f + (
		(0xFF - (pitch ? pitch : 0x7F)) >> 3 << 4)
	) / BAS.getFrequency();
}

void VM_Guest::renderAudio_C8(std::span<s16> buffer, const s16  amplitude) noexcept {
	for (auto& sample_s16 : buffer) {
		sample_s16 = mWavePhase > 0.5f ? amplitude : -amplitude;
		mWavePhase = std::fmod(mWavePhase + C8.mTone, 1.0f);
	}
}

/*==========================*/

VM_Guest::Audio_XO::Audio_XO(const BasicAudioSpec& BAS) noexcept
	: mStep{ 4000.0f / 128.0f / BAS.getFrequency() }
	, mTone{ mStep }
{}

void VM_Guest::setAudioTone_XO(const u8 pitch) noexcept {
	mAudioIsXO = true;
	XO.mTone = XO.mStep * std::pow(2.0f, (pitch - 64.0f) / 48.0f);
}

void VM_Guest::fetchPattern_XO() noexcept {
	mAudioIsXO = true;
	for (auto idx{ 0 }; idx < 16; ++idx) {
		XO.mData[idx] = readMemoryI(idx);
	}
}

void VM_Guest::renderAudio_XO(std::span<s16> buffer, const s16  amplitude) noexcept {
	for (auto& sample_s16 : buffer) {
		const auto step{ static_cast<s32>(std::clamp(mWavePhase * 128.0f, 0.0f, 127.0f)) };
		const auto mask{ 1 << (7 ^ step & 7) };
		sample_s16 = XO.mData[step >> 3] & mask ? amplitude : -amplitude;
		mWavePhase = std::fmod(mWavePhase + XO.mTone, 1.0f);
	}
}

/*==========================*/

void VM_Guest::resetAudioTrack() noexcept {
	mAudioIsMC   = false;
	MC.mTrackPos = MC.mStepping =
	MC.mTrackLen = MC.mMemPoint = 0;
}

void VM_Guest::startAudioTrack(const bool repeat) noexcept {

	MC.mTrackLen = readMemoryI(2) << 16
				 | readMemoryI(3) <<  8
				 | readMemoryI(4);

	if (!MC.mTrackLen) {
		resetAudioTrack();
		return;
	}

	mAudioIsMC   = true;
	MC.mStepping = (readMemoryI(0) << 8 | readMemoryI(1)) * 1.0 / BAS.getFrequency();
	MC.mTrackLen = repeat ? -MC.mTrackLen : MC.mTrackLen;
	MC.mTrackPos = 0.0;
	MC.mMemPoint = mRegisterI + 6;
}

void VM_Guest::renderAudio_MC(std::span<s16> buffer, s16 volume) noexcept {
	for (auto& sample_s16 : buffer) {
		sample_s16 = (readMemory(
			MC.mMemPoint + static_cast<u32>(MC.mTrackPos)
		) - 128) * volume;

		if ((MC.mTrackPos += MC.mStepping) >= std::abs(MC.mTrackLen)) {
			if (MC.mTrackLen < 0) {
				MC.mTrackPos += MC.mTrackLen;
			} else {
				resetAudioTrack();
				return;
			}
		}
	}
}

/*==========================*/

void VM_Guest::renderAudioData() {
	std::vector<s16> audioBuffer(static_cast<usz>(BAS.getFrequency() / mFramerate));

	if (mBuzzLight) {
		renderAudio_C8(audioBuffer, BAS.getAmplitude());
		BVS.setOutlineLitColor(Color.buzz[1]);
		BVS.setOutlineUnlitColor(Color.buzz[0]);
	} else if (mAudioIsMC) {
		renderAudio_MC(audioBuffer, BAS.getVolume());
		BVS.setOutlineLitColor(0xFF202020);
		BVS.setOutlineUnlitColor(0xFF202020);
	} else if (!mSoundTimer) {
		mWavePhase = 0.0f;
		BVS.setOutlineLitColor(Color.buzz[0]);
		BVS.setOutlineUnlitColor(Color.buzz[0]);
	} else if (mAudioIsXO) {
		renderAudio_XO(audioBuffer, BAS.getAmplitude());
		BVS.setOutlineLitColor(Color.buzz[0]);
		BVS.setOutlineUnlitColor(Color.buzz[0]);
	} else {
		renderAudio_C8(audioBuffer, BAS.getAmplitude());
		BVS.setOutlineLitColor(Color.buzz[1]);
		BVS.setOutlineUnlitColor(Color.buzz[0]);
	}

	BAS.pushAudioData(audioBuffer.data(), audioBuffer.size());
}

/*==================================================================*/
	#pragma endregion
/*==================================================================*/
