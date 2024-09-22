/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../GameBoy_CoreInterface.hpp"

#include <initializer_list>
#include <functional>

/*==================================================================*/

class GAMEBOY_CLASSIC final : public GameBoy_CoreInterface {
	static constexpr u32 cTotalMemory{  0x10000 };
	static constexpr u32 cSafezoneOOB{        8 };
	static constexpr f32 cRefreshRate{ 59.7275f };
	static constexpr s32 cResSizeMult{        2 };
	static constexpr s32 cScreenSizeX{      160 };
	static constexpr s32 cScreenSizeY{      144 };
	static constexpr s32 cCylesPerSec{  4194304 };
	static constexpr s32 cScreenSizeT{    23040 };

private:
	void instructionLoop() noexcept override;
	void renderAudioData() override;
	void renderVideoData() override;

	class MMU {
	public:

	
	std::array<u8, cTotalMemory> mMemoryBanks{};

		/* Memory Map */
		std::span<u8, 0x0100> mBootRomBank{ mMemoryBanks.begin() + 0x0000, 0x0100 }; // BOOT ROM
		std::span<u8, 0x4000> mRomBank00  { mMemoryBanks.begin() + 0x0000, 0x4000 }; // ROM BANK 0
		std::span<u8, 0x4000> mRomBankNN  { mMemoryBanks.begin() + 0x4000, 0x4000 }; // ROM BANK N
		std::span<u8, 0x2000> mVideoBank  { mMemoryBanks.begin() + 0x8000, 0x2000 }; // VRAM
		std::span<u8, 0x2000> mExtBank    { mMemoryBanks.begin() + 0xA000, 0x2000 }; // EXT RAM
		std::span<u8, 0x1000> mWorkBank0  { mMemoryBanks.begin() + 0xC000, 0x1000 }; // WRAM 0
		std::span<u8, 0x1000> mWorkBankN  { mMemoryBanks.begin() + 0xD000, 0x1000 }; // WRAM N
		std::span<u8, 0x1E00> mEchoBank   { mMemoryBanks.begin() + 0xE000, 0x1E00 }; // ECHO RAM (C000-DDFF)
		std::span<u8, 0x00A0> mObjAttrBank{ mMemoryBanks.begin() + 0xFE00, 0x00A0 }; // OAM
		//std::span<u8, 0x0060> mProhibited { mMemoryBanks.begin() + 0xFEA0, 0x0060 }; // PROHIBITED
		std::span<u8, 0x0080> mInOutBank  { mMemoryBanks.begin() + 0xFF00, 0x0080 }; // IO REGS
		std::span<u8, 0x007F> mHighBank   { mMemoryBanks.begin() + 0xFF80, 0x007F }; // HRAM


		/* Video Bank Map */
		std::span<u8, 0x1800> mVideoTileMap_{ mVideoBank.begin() + 0x0000, 0x1800 };
		std::span<u8, 0x0800> mVideoTileMap0{ mVideoBank.begin() + 0x0000, 0x0800 };
		std::span<u8, 0x0800> mVideoTileMap1{ mVideoBank.begin() + 0x0800, 0x0800 };
		std::span<u8, 0x0800> mVideoTileMap2{ mVideoBank.begin() + 0x1000, 0x0800 };

		std::span<u8, 0x0800> mVideoAttrMap_{ mVideoBank.begin() + 0x1800, 0x0800 };
		std::span<u8, 0x0400> mVideoAttrMap0{ mVideoBank.begin() + 0x1800, 0x0400 };
		std::span<u8, 0x0400> mVideoAttrMap1{ mVideoBank.begin() + 0x1C00, 0x0400 };


		/* I/O Ranges */
		u8& mJoyPadInput    { mInOutBank[0x00] }; // P1/JOYP
		u8& mSerialData     { mInOutBank[0x01] }; // SB
		u8& mSerialControl  { mInOutBank[0x02] }; // SC

		u8& mRegisterDIV    { mInOutBank[0x04] }; // DIV
		u8& mRegisterTIMA   { mInOutBank[0x05] }; // TIMA
		u8& mRegisterTMA    { mInOutBank[0x06] }; // TMA
		u8& mRegisterTAC    { mInOutBank[0x07] }; // TAC

		u8& mInterruptFlag  { mInOutBank[0x0F] }; // IF

		u8& mAudioCh1Sweep  { mInOutBank[0x10] }; // NR10
		u8& mAudioCh1Timer  { mInOutBank[0x11] }; // NR11
		u8& mAudioCh1VolEnv { mInOutBank[0x12] }; // NR12
		u8& mAudioCh1PerLo  { mInOutBank[0x13] }; // NR13
		u8& mAudioCh1PerHi  { mInOutBank[0x14] }; // NR14

		u8& mAudioCh2Timer  { mInOutBank[0x16] }; // NR21
		u8& mAudioCh2VolEnv { mInOutBank[0x17] }; // NR22
		u8& mAudioCh2PerLo  { mInOutBank[0x18] }; // NR23
		u8& mAudioCh2PerHi  { mInOutBank[0x19] }; // NR24

		u8& mAudioCh3Dac    { mInOutBank[0x1A] }; // NR30
		u8& mAudioCh3Timer  { mInOutBank[0x1B] }; // NR31
		u8& mAudioCh3VolEnv { mInOutBank[0x1C] }; // NR32
		u8& mAudioCh3PerLo  { mInOutBank[0x1D] }; // NR33
		u8& mAudioCh3PerHi  { mInOutBank[0x1E] }; // NR34

		u8& mAudioCh4Timer  { mInOutBank[0x20] }; // NR40
		u8& mAudioCh4FreqRng{ mInOutBank[0x21] }; // NR41
		u8& mAudioCh4PerHi  { mInOutBank[0x22] }; // NR42
		u8& mAudioCh4PerLo  { mInOutBank[0x23] }; // NR43

		u8& mAudioVolumeVIN { mInOutBank[0x24] }; // NR50
		u8& mAudioPanning   { mInOutBank[0x25] }; // NR51
		u8& mAudioEnable    { mInOutBank[0x26] }; // NR52

		std::span<u8, 0x10> mWavePattern
			{ mInOutBank.begin() + 0x30, 0x10 };

		std::span<u8, 0x0D> mDisplayControl
			{ mInOutBank.begin() + 0x40, 0x0D }; // split to regs here
		
		u8& mVideoBankSelect{ mInOutBank[0x4F] }; // VBK
		u8& mBootRomEnabled { mInOutBank[0x50] };
		u8& mVideoSrcHiDMA  { mInOutBank[0x51] }; // HDMA1
		u8& mVideoSrcLoDMA  { mInOutBank[0x52] }; // HDMA2
		u8& mVideoDstHiDMA  { mInOutBank[0x53] }; // HDMA3
		u8& mVideoDstLoDMA  { mInOutBank[0x54] }; // HDMA4
		u8& mVideoPropsDMA  { mInOutBank[0x55] }; // HDMA5
		u8& mInfraredPort   { mInOutBank[0x56] }; // RP
		u8& mBgPaletteSpec  { mInOutBank[0x68] }; // BCPS/BGPI
		u8& mBgPaletteData  { mInOutBank[0x69] }; // OCPS/OBPI
		u8& mObjPaletteSpec { mInOutBank[0x6A] }; // OCPD/OBPD
		u8& mObjPaletteData { mInOutBank[0x6B] }; // OPRI
		u8& mObjPriorityMode{ mInOutBank[0x6C] }; // SVBK
		u8& mWorkBankSelect { mInOutBank[0x70] }; // SVBK
		u8& mAudioDigiOut12 { mInOutBank[0x76] }; // PCM12
		u8& mAudioDigiOut32 { mInOutBank[0x77] }; // PCM34
		u8& mInterruptEnable{ mMemoryBanks[0xFFFF] }; // IE
	} mMMU;

	class PPU {

	} mPPU;


	class CPU {
		enum RegChar {
			A, B, C, D, E, F, H, L,
			AF, BC, DE, HL
		};
		class Registers {
			u8 mA{}, mB{}, mC{}, mD{};
			u8 mE{}, mF{}, mH{}, mL{};

		public:
			template <RegChar T>
			void set(const u32 value) noexcept {
				if constexpr (T == A) {
					mA = static_cast<u8>(value);
				} else if constexpr (T == B) {
					mB = static_cast<u8>(value);
				} else if constexpr (T == C) {
					mC = static_cast<u8>(value);
				} else if constexpr (T == D) {
					mD = static_cast<u8>(value);
				} else if constexpr (T == E) {
					mE = static_cast<u8>(value);
				} else if constexpr (T == F) {
					mF = static_cast<u8>(value);
				} else if constexpr (T == H) {
					mH = static_cast<u8>(value);
				} else if constexpr (T == L) {
					mL = static_cast<u8>(value);
				} else if constexpr (T == AF) {
					mA = static_cast<u8>(value >> 8);
					mF = static_cast<u8>(value >> 0);
				} else if constexpr (T == BC) {
					mB = static_cast<u8>(value >> 8);
					mC = static_cast<u8>(value >> 0);
				} else if constexpr (T == DE) {
					mD = static_cast<u8>(value >> 8);
					mE = static_cast<u8>(value >> 0);
				} else if constexpr (T == HL) {
					mH = static_cast<u8>(value >> 8);
					mL = static_cast<u8>(value >> 0);
				}
			}

			template <RegChar T>
			u32  get() const noexcept {
				if        constexpr (T == A) {
					return mA;
				} else if constexpr (T == B) {
					return mB;
				} else if constexpr (T == C) {
					return mC;
				} else if constexpr (T == D) {
					return mD;
				} else if constexpr (T == E) {
					return mE;
				} else if constexpr (T == F) {
					return mF;
				} else if constexpr (T == H) {
					return mH;
				} else if constexpr (T == L) {
					return mL;
				} else if constexpr (T == AF) {
					return mA << 8 | mF;
				} else if constexpr (T == BC) {
					return mB << 8 | mC;
				} else if constexpr (T == DE) {
					return mD << 8 | mE;
				} else if constexpr (T == HL) {
					return mH << 8 | mL;
				}
			}

			bool getFlagZ() const noexcept { return mF & 0x80; }
			bool getFlagN() const noexcept { return mF & 0x40; }
			bool getFlagH() const noexcept { return mF & 0x20; }
			bool getFlagC() const noexcept { return mF & 0x10; }

			void setFlagZ(const bool state) noexcept { mF = (mF & ~0x8F) | (state << 7); }
			void setFlagN(const bool state) noexcept { mF = (mF & ~0x4F) | (state << 6); }
			void setFlagH(const bool state) noexcept { mF = (mF & ~0x2F) | (state << 5); }
			void setFlagC(const bool state) noexcept { mF = (mF & ~0x1F) | (state << 4); }
		} mReg;

		u16  currentPC{};
		u8*  stackPtr{};
		bool mIME{ true };

	} mCPU;


	struct Opcode {
		using InstrStep = std::function<void(*)(CPU* const, MMU* const)>;

		const u32 mOpcode{};
		const u32 mLength{};
		const u32 mCyclesT{};
		const u32 mCyclesM{};
		std::vector<InstrStep>
			mInstrSteps{};

		Opcode(
			const u32 opcode, const u32 length, const u32 cyclesT,
			std::initializer_list<InstrStep> steps
		) noexcept
			: mOpcode{ opcode }
			, mLength{ length }
			, mCyclesT{ cyclesT }
			, mCyclesM{ cyclesT / 4 }
			, mInstrSteps{ steps }
		{}
	};


public:
	GAMEBOY_CLASSIC();

	static constexpr bool testGameSize(const usz size) noexcept {
		return size <= 0;
	}
};
