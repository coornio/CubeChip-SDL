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

bool VM_Guest::isSystemStopped() const { return mSystemStopped || mCyclesPerFrame == 0; }
void VM_Guest::isSystemStopped(const bool state) { mSystemStopped = state; }

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
				case 0x00B0:
				case 0x00D0:
					instruction_00DN_XO(N);
					break;
				case 0x00C0:
					instruction_00CN_XO(N);
					break;
				case 0x00E0: switch (N) {
					case 0x0:
						if (isManualRefresh()) {
							instruction_00E0_MC();
						} else if (isPixelBitColor()) {
							instruction_00E0_XO();
						} else {
							instruction_00E0_C8();
						}
						break;
					case 0x1:
						instruction_00E1_HW();
						break;
					case 0xD:
						instruction_00ED_8E();
						break;
					case 0xE:
						instruction_00EE_C8();
						break;
					[[unlikely]] default: triggerOpcodeError(mInstruction);
				} break;
				case 0x00F0: switch (N) {
					case 0x0:
						if (routineReturn()) [[unlikely]]
							{ triggerError("Error :: Cannot return from empty stack!"); }
						break;
					case 0x1:
						instruction_00F1_HW();
						break;
					case 0x2:
						instruction_00F2_HW();
						break;
					case 0x3:
						instruction_00F3_HW();
						break;
					case 0xB:
						instruction_00FB_XO(4);
						break;
					case 0xC:
						instruction_00FC_XO(4);
						break;
					case 0xD:
						instruction_00FD_C8();
						break;
					case 0xE:
						if (!isManualRefresh()) [[likely]] {
							instruction_00FE_C8();
						}
						break;
					case 0xF:
						if (!isManualRefresh()) [[likely]] {
							instruction_00FF_C8();
						}
						break;
					[[unlikely]] default: triggerOpcodeError(mInstruction);
				} break;
				default: {
					if (State.megachip_rom || State.gigachip_rom) {
						switch (X) {
							case 0x0: switch (LO) {
								case 0x10:
									instruction_0010_MC(BVS.getFrameColor());
									break;
								case 0x11:
									instruction_0011_MC();
									break;
								[[unlikely]] default: triggerOpcodeError(mInstruction);
							} break;
							case 0x1:
								instruction_01NN_MC(LO);
								break;
							case 0x2:
								instruction_02NN_MC(LO);
								break;
							case 0x3:
								instruction_03NN_MC(LO);
								break;
							case 0x4:
								instruction_04NN_MC(LO);
								break;
							case 0x5:
								instruction_05NN_MC(LO);
								break;
							case 0x6:
								if (Y) [[unlikely]] { triggerOpcodeError(mInstruction); }
								else { instruction_060N_MC(N); }
								break;
							case 0x7:
								if (LO) [[unlikely]] { triggerOpcodeError(mInstruction); }
								else { instruction_0700_MC(); }
								break;
							case 0x8:
								if (Y) [[unlikely]] { triggerOpcodeError(mInstruction); }
								else if (State.gigachip_rom) {
									instruction_080N_GC(N);
								} else {
									instruction_080N_MC(N);
								}
								break;
							case 0x9:
								instruction_09NN_MC(LO);
								break;
							[[unlikely]] default: triggerOpcodeError(mInstruction);
						}
					}
					else switch (NNN()) {
						case 0x151:
							instruction_0151_8E();
							break;
						case 0x188:
							instruction_0188_8E();
							break;
						case 0x216:
							instruction_0216_C8_4P();
							break;
						case 0x200:
							instruction_0200_C8_4P();
							break;
						case 0x230:
							instruction_0200_C8_2P();
							break;
						case 0x2A0:
							instruction_02A0_8X();
							break;
						case 0x2F0:
							instruction_02F0_8X_MP();
							break;
						[[unlikely]] default: triggerOpcodeError(mInstruction);
					}
				}
			} break;
			case 0x1:
				instruction_1NNN_C8();
				break;
			case 0x2:
				instruction_2NNN_C8();
				break;
			case 0x3:
				instruction_3xNN_C8(X, LO);
				break;
			case 0x4:
				instruction_4xNN_C8(X, LO);
				break;
			case 0x5: switch (N) {
				case 0x0:
					instruction_5xy0_C8(X, Y);
					break;
				case 0x1:
					if (State.chip8X_rom) {
						instruction_5xy1_8X(X, Y);
					} else {
						instruction_5xy1_8E(X, Y);
					}
					break;
				case 0x2: {
					if (State.chip8E_rom) [[unlikely]] {
						if (X < Y) {
							instruction_5xy2_8E(X, Y);
						} else [[unlikely]]
							{ triggerOpcodeError(mInstruction); }
					} else {
						instruction_5xy2_XO(X, Y);
					}
				} break;
				case 0x3: {
					if (State.chip8E_rom) [[unlikely]] {
						if (X < Y) {
							instruction_5xy3_8E(X, Y);
						} else [[unlikely]]
							{ triggerOpcodeError(mInstruction); }
					} else {
						instruction_5xy3_XO(X, Y);
					}
				} break;
				case 0x4: {
					instruction_5xy4_XO(X, Y);
				} break;
				[[unlikely]] default: triggerOpcodeError(mInstruction);
			} break;
			case 0x6:
				instruction_6xNN_C8(X, LO);
				break;
			case 0x7:
				instruction_7xNN_C8(X, LO);
				break;
			case 0x8: switch (N) {
				case 0x0:
					instruction_8xy0_C8(X, Y);
					break;
				case 0x1:
					instruction_8xy1_C8(X, Y);
					break;
				case 0x2:
					instruction_8xy2_C8(X, Y);
					break;
				case 0x3:
					instruction_8xy3_C8(X, Y);
					break;
				case 0x4: {
					instruction_8xy4_C8(X, Y);
				} break;
				case 0x5: {
					instruction_8xy5_C8(X, Y);
				} break;
				case 0x7: {
					instruction_8xy7_C8(X, Y);
				};  break;
				case 0x6: {
					instruction_8xy6_C8(X, Y);
				} break;
				case 0xE: {
					instruction_8xyE_C8(X, Y);
				} break;
				case 0xC: {
					instruction_8xyC_HW(X, Y);
				} break;
				case 0xD: {
					instruction_8xyD_HW(X, Y);
				} break;
				case 0xF: {
					instruction_8xyF_HW(X, Y);
				} break;
				[[unlikely]] default: triggerOpcodeError(mInstruction);
			} break;
			case 0x9: switch (N) {
				case 0x0:
					instruction_9xy0_C8(X, Y);
					break;
				[[unlikely]] default: triggerOpcodeError(mInstruction);
			} break;
			case 0xA:
				instruction_ANNN_C8();
				break;
			case 0xB: {
				if (State.chip8E_rom) {
					switch (X) {
						case 0xB:
							instruction_BBNN_8E(LO);
							break;
						case 0xF:
							instruction_BFNN_8E(LO);
							break;
						[[unlikely]] default: triggerOpcodeError(mInstruction);
					}
				}
				else if (State.chip8X_rom) {
					if (X == 0xF) [[unlikely]] {
						triggerOpcodeError(mInstruction);
					} else {
						instruction_BxyN_8X(X, Y, N);
					}
				} else {
					instruction_BxNN_C8(X);
				}
			} break;
			case 0xC:
				instruction_CxNN_C8(X, LO);
				break;
			case 0xD:
				instruction_DxyN_C8(X, Y, N);
				break;
			case 0xE: switch (LO) {
				case 0x9E:
					instruction_Ex9E_C8(X);
					break;
				case 0xA1:
					instruction_ExA1_C8(X);
					break;
				case 0xF2:
					instruction_ExF2_8X(X);
					break;
				case 0xF5:
					instruction_ExF5_8X(X);
					break;
				[[unlikely]] default: triggerOpcodeError(mInstruction);
			} break;
			case 0xF: switch (NNN()) {
				case 0x000:
					instruction_F000_XO();
					break;
				case 0x002:
					instruction_F002_XO();
					break;
				case 0x100:
					instruction_F100_HW();
					break;
				case 0x200:
					instruction_F200_HW();
					break;
				case 0x300:
					instruction_F300_HW();
					break;
				default: switch (LO) {
					case 0x01:
						instruction_Fx01_XO(X);
						break;
					case 0x03:
						if (State.chip8E_rom) [[unlikely]] {
							instruction_Fx03_8X(X);
						} else {
							instruction_Fx03_HW(X);
						}
						break;
					case 0x07:
						instruction_Fx07_C8(X);
						break;
					case 0x0A:
						instruction_Fx0A_C8(X);
						break;
					case 0x15:
						instruction_Fx15_C8(X);
						break;
					case 0x18:
						instruction_Fx18_C8(X);
						break;
					case 0x1B:
						instruction_Fx1B_8E(X);
						break;
					case 0x1E:
						instruction_Fx1E_C8(X);
						break;
					case 0x1F:
						instruction_Fx1F_HW(X);
						break;
					case 0x29:
						instruction_Fx29_C8(X);
						break;
					case 0x30:
						instruction_Fx30_C8(X);
						break;
					case 0x33:
						instruction_Fx33_C8(X);
						break;
					case 0x3A:
						instruction_Fx3A_XO(X);
						break;
					case 0x4F:
						instruction_Fx4F_8E(X);
						break;
					case 0x55:
						instruction_Fx55_C8(X);
						break;
					case 0x65:
						instruction_Fx65_C8(X);
						break;
					case 0x75:
						instruction_Fx75_C8(X);
						break;
					case 0x85:
						instruction_Fx85_C8(X);
						break;
					case 0xE3:
						instruction_FxE3_8E(X);
						break;
					case 0xE7:
						instruction_FxE7_8E(X);
						break;
					case 0xF8:
						instruction_FxF8_8X(X);
						break;
					case 0xFB:
						instruction_FxFB_8X(X);
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
	if (isSystemStopped()) { return; }
	else { ++mTotalFrames; }

	Input.updateKeyStates();

	if ( mDelayTimer) { --mDelayTimer; }
	if ( mSoundTimer) { --mSoundTimer; }
	if (!mSoundTimer) { mBuzzLight = false; }

	handleFrameWait();

	instructionLoop();

	handleInputWait();

	renderAudioData();

	if (isManualRefresh()) { return; }

	renderToTexture();
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


void VM_Guest::flushBuffers(const FlushType option) {
	switch (option) {
		case FlushType::DISCARD:
			megaColorPalette.wipeAll();
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

void VM_Guest::setBackgroundColorTo(const u32 color) const noexcept {
	BVS.setFrameColor(color);
}

void VM_Guest::cycleBackgroundColor() noexcept {
	BVS.setFrameColor(Color.BackColors[Color.bgindex++ & 0x3]);
}

void VM_Guest::setDisplayOpacity(const s32 value) const {
	BVS.setTextureAlpha(value);
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
