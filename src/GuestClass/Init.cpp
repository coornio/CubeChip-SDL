/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <fstream>
#include <execution>
#include <cstddef>
#include <cstdint>

#include "../Assistants/BasicLogger.hpp"
using namespace blogger;

#include "../HostClass/BasicVideoSpec.hpp"
#include "../HostClass/HomeDirManager.hpp"

#include "Guest.hpp"
#include "GameFileChecker.hpp"
#include "HexInput.hpp"

bool MEGACORE::setupMachine() {
	if (!romTypeCheck()) {
		return false;
	}

	initPlatform();
	fontCopyToMemory();

	blog.newEntry(BLOG::INFO, "Successfully initialized rom/platform.");
	return true;
}

bool MEGACORE::romTypeCheck() {
	/*
	* This place requires a database check, only after which would
	* we fall back to deriving the platform specifics via extension
	*/
	switch (cexprHash(HDM.getFileExts().c_str())) {

		case (FileExt::c2x):
			if (!romCopyToMemory(4'096, 0x300))
				return false;
			initProgramParams(0x300, 30);
			changeFunctionSet(&SetClassic8);
			State.chip8X_rom   = true;
			State.chip8_legacy = true;
			State.hires_2paged = true;
			break;

		case (FileExt::c4x):
			if (!romCopyToMemory(4'096, 0x300))
				return false;
			initProgramParams(0x300, 30);
			changeFunctionSet(&SetClassic8);
			State.chip8X_rom   = true;
			State.chip8_legacy = true;
			State.hires_4paged = true;
			break;

		case (FileExt::c8x):
			if (!romCopyToMemory(4'096, 0x300))
				return false;
			initProgramParams(0x300, 30);
			changeFunctionSet(&SetClassic8);
			State.chip8_legacy = true;
			State.chip8X_rom   = true;
			break;

		case (FileExt::c8e):
			if (!romCopyToMemory(4'096, 0x200))
				return false;
			initProgramParams(0x200, 30);
			changeFunctionSet(&SetClassic8);
			State.chip8_legacy = true;
			State.chip8E_rom   = true;
			break;

		case (FileExt::c2h):
			if (!romCopyToMemory(4'096, 0x260))
				return false;
			initProgramParams(0x260, 30);
			changeFunctionSet(&SetClassic8);
			State.chip8_legacy = true;
			State.hires_2paged = true;
			break;

		case (FileExt::c4h):
			if (!romCopyToMemory(4'096, 0x244))
				return false;
			initProgramParams(0x244, 30);
			changeFunctionSet(&SetClassic8);
			State.chip8_legacy = true;
			State.hires_4paged = true;
			break;

		case (FileExt::c8h):
			if (!romCopyToMemory(4'096, 0x200))
				return false;
			if (readMemory(0x200) != 0x12 || readMemory(0x201) != 0x60) {
				blog.newEntry(BLOG::WARN, "Invalid TPD rom patch, aborting.");
				return false;
			}
			initProgramParams(0x2C0, 30);
			changeFunctionSet(&SetClassic8);
			State.chip8_legacy = true;
			State.hires_2paged = true;
			Quirk.idxRegNoInc  = true;
			Quirk.shiftVX      = true;
			break;

		case (FileExt::ch8):
			if (!romCopyToMemory(4'096, 0x200))
				return false;
			initProgramParams(0x200, 11);
			changeFunctionSet(&SetClassic8);
			break;

		case (FileExt::sc8):
			if (!romCopyToMemory(4'096, 0x200))
				return false;
			initProgramParams(0x200, 30);
			changeFunctionSet(&SetClassic8);
			break;

		case (FileExt::gc8):
			if (!romCopyToMemory(16'777'216, 0x200))
				return false;
			initProgramParams(0x200, 10'000);
			changeFunctionSet(&SetGigachip);
			State.gigachip_rom = true;
			break;

		case (FileExt::mc8):
			if (!romCopyToMemory(16'777'216, 0x200))
				return false;
			initProgramParams(0x200, 3'000);
			changeFunctionSet(&SetMegachip);
			State.megachip_rom = true;
			Quirk.waitScroll   = true;
			Quirk.idxRegNoInc  = true;
			Quirk.shiftVX      = true;
			Quirk.jmpRegX      = true;
			break;

		case (FileExt::xo8):
		case (FileExt::hw8):
			if (!romCopyToMemory(65'536, 0x200))
				return false;
			initProgramParams(0x200, 200'000);
			changeFunctionSet(&SetModernXO);
			isPixelBitColor(true);
			Quirk.wrapSprite = true;
			break;

		case (FileExt::bnc):
			if (!romCopyToMemory(65'536, 0x200))
				return false;
			initProgramParams(0x200, 4'000'000);
			changeFunctionSet(&SetClassic8);
			break;

		default:
			blog.newEntry(BLOG::WARN, "Unknown rom type, we shouldn't be here!");
			return false;
	}
	return true;
}

bool MEGACORE::romCopyToMemory(const usz size, const usz offset) {
	mMemoryBank.resize(size);

	std::basic_ifstream<char> ifs(HDM.getFilePath(), std::ios::binary);
	ifs.read(reinterpret_cast<char*>(mMemoryBank.data() + offset), HDM.getFileSize());
	if (ifs.fail()) {
		blog.newEntry(BLOG::ERROR, "Failed to copy rom data to memory, aborting.");
		return false;
	} else {
		return true;
	}
}

void MEGACORE::initPlatform() {
	//Color->bit[0] = 0xFF000000;
	//Color->bit[1] = 0xFF333333;
	//Color->bit[2] = 0xFF555555;
	//Color->bit[3] = 0xFFFFFFFF;
	//Quirk.clearVF = true;
	//Quirk.shiftVX = true;
	//Quirk.jmpRegX = true;
	//Quirk.idxRegNoInc = true;
	//State.chip8_legacy = true;
	//State.schip_legacy = true;
	//Display->isPixelBitColor(true);
	//Display->isPixelTrailing(true);
	calculateBoostCPF(0);
	//Quirk.waitScroll = true;
	//Quirk.waitVblank = true;
	//Quirk.wrapSprite = true;
	Input.loadPresetBinds();

	// XXX - apply custom rom settings here

	if (State.hires_2paged || State.hires_4paged) {
		isPixelBitColor(false);
		State.schip_legacy = false;
	}
	if (State.megachip_rom) {
		isPixelTrailing(false);
		isPixelBitColor(false);
		mFramerate = 60.0f;
		State.chip8_legacy = false;
		State.schip_legacy = false;
	}
	if (State.chip8_legacy) {
		isPixelTrailing(true);
		Quirk.clearVF    = true;
		Quirk.waitVblank = true;
	}
	if (State.schip_legacy) {
		changeFunctionSet(&SetLegacySC);
		mFramerate = 64.0f; // match HP48 framerate
		isPixelTrailing(true);
		Quirk.shiftVX      = true;
		Quirk.jmpRegX      = true;
		Quirk.idxRegNoInc  = true;
	}

	if (State.hires_2paged) {
		prepDisplayArea(Resolution::TP, true);
	}
	else if (State.hires_4paged) {
		prepDisplayArea(Resolution::FP, true);
	}
	else {
		prepDisplayArea(Resolution::LO, true);
	}

	if (State.chip8X_rom) {
		cycleBackgroundColor();
		color8xBuffer.resize(true, Trait.H, Trait.W >> 3);
		color8xBuffer.at_raw(0, 0) = getForegroundColor8X(2);

		if (!State.schip_legacy) return;

		color8xBuffer.at_raw(4, 1) =
		color8xBuffer.at_raw(4, 0) =
		color8xBuffer.at_raw(0, 1) =
		color8xBuffer.at_raw(0, 0);
	}
}

void MEGACORE::prepDisplayArea(const Resolution mode, const bool forced) {
	//                                HI   LO   TP   FP   MC
	static constexpr s32 sizeW[]{ 0, 128,  64,  64,  64, 256 };
	static constexpr s32 sizeH[]{ 0,  64,  32,  64, 128, 192 };

	const auto select{ State.schip_legacy ? 1 : static_cast<u32>(mode) };
	const auto lores{ mode == Resolution::LO }; isLoresExtended(lores);

	const auto W{ sizeW[select] };
	const auto H{ sizeH[select] };

	if (W != Trait.W || H != Trait.H) {
		BVS.createTexture(W, H);
	}

	Trait.W = W; Trait.Wb = W - 1;
	Trait.H = H; Trait.Hb = H - 1;
	Trait.S = W * H;

	if (isManualRefresh()) {
		BVS.setAspectRatio(512, 384, -2);
		foregroundBuffer.resize(false, H, W);
		backgroundBuffer.resize(false, H, W);
		collisionPalette.resize(false, H, W);
		megaColorPalette.resize(false, 1, 256);
	} else {
		BVS.setAspectRatio(512, 256, +2);
		displayBuffer[0].resize(!forced, H, W);
		if (isPixelBitColor() || isPixelTrailing()) {
			displayBuffer[1].resize(!forced, H, W);
			displayBuffer[2].resize(!forced, H, W);
			displayBuffer[3].resize(!forced, H, W);
		}
	}

	if (islegacyPlatform() && (forced || Quirk.waitVblank ^ lores)) {
		//Program->mCyclesPerFrame     += Program->boost *= -1;
		//Quirk.waitVblank  = lores;
		//printf("mCyclesPerFrame: %d\n", Program->mCyclesPerFrame);
	}
};

void MEGACORE::fontCopyToMemory() {
	static constexpr u8 FONT_DATA[]{
		0x60, 0xA0, 0xA0, 0xA0, 0xC0, // 0
		0x40, 0xC0, 0x40, 0x40, 0xE0, // 1
		0xC0, 0x20, 0x40, 0x80, 0xE0, // 2
		0xC0, 0x20, 0x40, 0x20, 0xC0, // 3
		0x20, 0xA0, 0xE0, 0x20, 0x20, // 4
		0xE0, 0x80, 0xC0, 0x20, 0xC0, // 5
		0x40, 0x80, 0xC0, 0xA0, 0x40, // 6
		0xE0, 0x20, 0x60, 0x40, 0x40, // 7
		0x40, 0xA0, 0x40, 0xA0, 0x40, // 8
		0x40, 0xA0, 0x60, 0x20, 0x40, // 9
		0x40, 0xA0, 0xE0, 0xA0, 0xA0, // A
		0xC0, 0xA0, 0xC0, 0xA0, 0xC0, // B
		0x60, 0x80, 0x80, 0x80, 0x60, // C
		0xC0, 0xA0, 0xA0, 0xA0, 0xC0, // D
		0xE0, 0x80, 0xC0, 0x80, 0xE0, // E
		0xE0, 0x80, 0xC0, 0x80, 0x80, // F

		0x7C, 0xC6, 0xCE, 0xDE, 0xD6, 0xF6, 0xE6, 0xC6, 0x7C, 0x00, // 0
		0x10, 0x30, 0xF0, 0x30, 0x30, 0x30, 0x30, 0x30, 0xFC, 0x00, // 1
		0x78, 0xCC, 0xCC, 0x0C, 0x18, 0x30, 0x60, 0xCC, 0xFC, 0x00, // 2
		0x78, 0xCC, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0xCC, 0x78, 0x00, // 3
		0x0C, 0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x0C, 0x1E, 0x00, // 4
		0xFC, 0xC0, 0xC0, 0xC0, 0xF8, 0x0C, 0x0C, 0xCC, 0x78, 0x00, // 5
		0x38, 0x60, 0xC0, 0xC0, 0xF8, 0xCC, 0xCC, 0xCC, 0x78, 0x00, // 6
		0xFE, 0xC6, 0xC6, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00, // 7
		0x78, 0xCC, 0xCC, 0xEC, 0x78, 0xDC, 0xCC, 0xCC, 0x78, 0x00, // 8
		0x7C, 0xC6, 0xC6, 0xC6, 0x7C, 0x18, 0x18, 0x30, 0x70, 0x00, // 9
		//----------- omit segment below if legacy superchip -----------
		0x30, 0x78, 0xCC, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0xCC, 0x00, // A
		0xFC, 0x66, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x66, 0xFC, 0x00, // B
		0x3C, 0x66, 0xC6, 0xC0, 0xC0, 0xC0, 0xC6, 0x66, 0x3C, 0x00, // C
		0xF8, 0x6C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00, // D
		0xFE, 0x62, 0x60, 0x64, 0x7C, 0x64, 0x60, 0x62, 0xFE, 0x00, // E
		0xFE, 0x66, 0x62, 0x64, 0x7C, 0x64, 0x60, 0x60, 0xF0, 0x00, // F
	};

	static constexpr u8 MEGA_FONT_DATA[]{
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, // 1
		0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, // 2
		0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, // 3
		0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, // 4
		0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, // 5
		0x3E, 0x7C, 0xC0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, // 6
		0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, // 7
		0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, // 8
		0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C, // 9
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C  // 0
	};

	// copy the FONT at the desired offset, and omit A-F superchip sprites if needed
	std::copy_n(
		std::execution::unseq,
		FONT_DATA, State.schip_legacy ? 180 : 240,
		mMemoryBank.data()
	);

	if (!State.megachip_rom) return;

	std::copy_n(
		std::execution::unseq,
		MEGA_FONT_DATA, 160,
		mMemoryBank.data() + 240
	);
}

void MEGACORE::renderToTexture() {
	if (isManualRefresh()) {
		std::copy_n(
			std::execution::seq,
			foregroundBuffer.data(),
			Trait.S, BVS.lockTexture()
		);
	}
	else if (isPixelBitColor()) {
		auto* pixels{ BVS.lockTexture() };
		for (auto idx{ 0 }; idx < Trait.S; ++idx) {
			pixels[idx] = (0xFF << 24 | Color.bit[
				displayBuffer[0].at_raw(idx) << 0 |
				displayBuffer[1].at_raw(idx) << 1 |
				displayBuffer[2].at_raw(idx) << 2 |
				displayBuffer[3].at_raw(idx) << 3
			]);
		}
	}
	else if (State.chip8X_rom) {
		auto* pixels{ BVS.lockTexture() };
		if (isPixelTrailing()) {
			for (auto idx{ 0 }; idx < Trait.S; ++idx) {
				const auto Y = idx / Trait.W & Trait.mask8X;
				const auto X = idx % Trait.W >> 3; // 8px color zones

				if (displayBuffer[0].at_raw(idx)) {
					pixels[idx] = (0xFF << 24 | color8xBuffer.at_raw(Y, X));
					continue;
				}
				if (displayBuffer[1].at_raw(idx)) {
					pixels[idx] = (0xE8 << 24 | color8xBuffer.at_raw(Y, X));
					continue;
				}
				if (displayBuffer[2].at_raw(idx)) {
					pixels[idx] = (0x7B << 24 | color8xBuffer.at_raw(Y, X));
					continue;
				}
				if (displayBuffer[3].at_raw(idx)) {
					pixels[idx] = (0x38 << 24 | color8xBuffer.at_raw(Y, X));
					continue;
				} else {
					pixels[idx] = 0;
				}
			}
			displayBuffer[3] = displayBuffer[2];
			displayBuffer[2] = displayBuffer[1];
			displayBuffer[1] = displayBuffer[0];
		} else {
			for (auto idx{ 0 }; idx < Trait.S; ++idx) {
				const auto Y = idx / Trait.W & Trait.mask8X;
				const auto X = idx % Trait.W >> 3; // 8px color zones
			
				pixels[idx] = (0xFF << 24 | (displayBuffer[0].at_raw(idx)
					? color8xBuffer.at_raw(Y, X) : 0));
			}
		}
	}
	else {
		if (isPixelTrailing()) {
			auto* pixels{ BVS.lockTexture() };
			for (auto idx{ 0 }; idx < Trait.S; ++idx) {
				if (displayBuffer[0].at_raw(idx)) {
					pixels[idx] = (0xFF << 24 | Color.bit[1]);
					continue;
				}
				if (displayBuffer[1].at_raw(idx)) {
					pixels[idx] = (0xE8 << 24 | Color.bit[1]);
					continue;
				}
				if (displayBuffer[2].at_raw(idx)) {
					pixels[idx] = (0x7B << 24 | Color.bit[1]);
					continue;
				}
				if (displayBuffer[3].at_raw(idx)) {
					pixels[idx] = (0x38 << 24 | Color.bit[1]);
					continue;
				} else {
					pixels[idx] = 0;
				}
			}
			displayBuffer[3] = displayBuffer[2];
			displayBuffer[2] = displayBuffer[1];
			displayBuffer[1] = displayBuffer[0];
		} else {
			std::transform(
				std::execution::unseq,
				displayBuffer[0].raw_begin(),
				displayBuffer[0].raw_end(),
				BVS.lockTexture(),
				[this](const auto pixel) noexcept {
					return 0xFF << 24 | Color.bit[pixel];
				}
			);
		}
	}
	BVS.unlockTexture();
}
