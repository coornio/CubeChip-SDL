/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <array>
#include <fstream>
#include <cstddef>
#include <cstdint>

#include "../Assistants/BasicLogger.hpp"
using namespace blogger;

#include "../HostClass/BasicVideoSpec.hpp"
#include "../HostClass/HomeDirManager.hpp"

#include "Guest.hpp"
#include "FileTypes.hpp"
#include "HexInput.hpp"
#include "ProgramControl.hpp"
#include "MemoryBanks.hpp"
#include "DisplayColors.hpp"

bool VM_Guest::setupMachine() {
	if (!romTypeCheck()) {
		return false;
	}

	initPlatform();
	loadFontData();

	blog.stdLogOut("Successfully initialized rom/platform.");
	return true;
}

bool VM_Guest::romTypeCheck() {
	/*
	* This place requires a database check, only after which would
	* we fall back to deriving the platform specifics via extension
	*/
	switch (HDM->hash) {

		case (FileTypes::c2x):
			if (!loadRomToRam(4'096, 0x300))
				return false;
			Program->init(0x300, 15);
			Program->setFncSet(&SetClassic8);
			State.chip8X_rom   = true;
			State.hires_2paged = true;
			break;

		case (FileTypes::c4x):
			if (!loadRomToRam(4'096, 0x300))
				return false;
			Program->init(0x300, 15);
			Program->setFncSet(&SetClassic8);
			State.chip8X_rom   = true;
			State.hires_4paged = true;
			break;

		case (FileTypes::c8x):
			if (!loadRomToRam(4'096, 0x300))
				return false;
			Program->init(0x300, 30);
			Program->setFncSet(&SetClassic8);
			State.chip8X_rom = true;
			break;

		case (FileTypes::c8e):
			if (!loadRomToRam(4'096, 0x200))
				return false;
			Program->init(0x200, 30);
			Program->setFncSet(&SetClassic8);
			State.chip8E_rom = true;
			break;

		case (FileTypes::c2h):
			if (!loadRomToRam(4'096, 0x260))
				return false;
			Program->init(0x260, 15);
			Program->setFncSet(&SetClassic8);
			State.chip_classic = true;
			State.hires_2paged = true;
			break;

		case (FileTypes::c4h):
			if (!loadRomToRam(4'096, 0x244))
				return false;
			Program->init(0x244, 15);
			Program->setFncSet(&SetClassic8);
			State.chip_classic = true;
			State.hires_4paged = true;
			break;

		case (FileTypes::c8h):
			if (!loadRomToRam(4'096, 0x200))
				return false;
			if (mrw(0x200) != 0x12 || mrw(0x201) != 0x60) {
				blog.stdLogOut("Invalid TPD rom patch, aborting.");
				return false;
			}
			Program->init(0x2C0, 30);
			Program->setFncSet(&SetClassic8);
			State.chip_classic = true;
			State.hires_2paged = true;
			Quirk.idxRegNoInc  = true;
			Quirk.shiftVX      = true;
			break;

		case (FileTypes::ch8):
			if (!loadRomToRam(4'096, 0x200))
				return false;
			Program->init(0x200, 11);
			Program->setFncSet(&SetClassic8);
			State.chip_classic = true;
			break;

		case (FileTypes::sc8):
			if (!loadRomToRam(4'096, 0x200))
				return false;
			Program->init(0x200, 30);
			Program->setFncSet(&SetClassic8);
			State.chip_classic = true;
			break;

		case (FileTypes::gc8):
			if (!loadRomToRam(16'777'216, 0x200))
				return false;
			Program->init(0x200, 10'000);
			Program->setFncSet(&SetGigachip);
			State.gigachip_rom = true;
			break;

		case (FileTypes::mc8):
			if (!loadRomToRam(16'777'216, 0x200))
				return false;
			Program->init(0x200, 3'000);
			Program->setFncSet(&SetMegachip);
			State.megachip_rom = true;
			break;

		case (FileTypes::xo8):
		case (FileTypes::hw8):
			if (!loadRomToRam(65'536, 0x200))
				return false;
			Program->init(0x200, 200'000);
			Program->setFncSet(&SetModernXO);
			State.xochip_color = true;
			Quirk.wrapSprite   = true;
			break;

		default:
			if (!loadRomToRam(4'096, 0x200))
				return false;
			Program->init(0x200, 2'900'000);
			Program->setFncSet(&SetClassic8);
			State.chip_classic = true;
			blog.stdLogOut("Unknown rom type, default parameters apply.");
	}
	return true;
}

bool VM_Guest::loadRomToRam(const std::size_t size, const std::size_t offset) {
	Program->limiter = size - 1; // set program memory limit
	Mem->memory.resize(size);    // resize the memory vector

	std::basic_ifstream<char> ifs(HDM->path, std::ios::binary);
	ifs.read(reinterpret_cast<char*>(&Mem->memory[offset]), HDM->size);
	return !ifs.fail();
}

void VM_Guest::initPlatform() {
	//Color->bit[0] = 0xFF000000;
	//Color->bit[1] = 0xFF333333;
	//Color->bit[2] = 0xFF555555;
	//Color->bit[3] = 0xFFFFFFFF;
	//Quirk.clearVF = true;
	//Quirk.shiftVX = true;
	//Quirk.jmpRegX = true;
	//Quirk.idxRegNoInc = true;
	//State.schip_legacy = true;
	//State.xochip_color = true;
	Program->setSpeed(0);
	//Quirk.waitScroll = true;
	//Quirk.waitVblank = true;
	//Quirk.wrapSprite = true;
	Input->reset();

	// XXX - apply custom rom settings here

	if (State.hires_2paged || State.hires_4paged) {
		State.xochip_color = false;
		State.schip_legacy = false;
	}
	if (State.megachip_rom) {
		State.xochip_color = false;
		State.schip_legacy = false;
	}
	if (State.chip8_legacy) {
		Quirk.clearVF    = true;
		Quirk.waitVblank = true;
	}
	if (State.schip_legacy) {
		Program->setFncSet(&SetLegacySC);
		Program->framerate = 64.0; // match HP48 framerate
		Quirk.shiftVX      = true;
		Quirk.jmpRegX      = true;
		Quirk.idxRegNoInc  = true;
	}

	if (State.hires_2paged) {
		setupDisplay(Resolution::TP, true);
	}
	else if (State.hires_4paged) {
		setupDisplay(Resolution::FP, true);
	}
	else {
		setupDisplay(Resolution::LO, true);
	}

	if (State.chip8X_rom) {
		Color->cycleBackground();
		Mem->color8xBuffer.resize(true, Plane.H, Plane.W >> 3);
		Mem->color8xBuffer.at_raw(0, 0) = Color->getFore8X(2);

		if (!State.schip_legacy) return;

		Mem->color8xBuffer.at_raw(4, 1) =
		Mem->color8xBuffer.at_raw(4, 0) =
		Mem->color8xBuffer.at_raw(0, 1) =
		Mem->color8xBuffer.at_raw(0, 0);
	}
}

void VM_Guest::setupDisplay(const Resolution mode, const bool forced) {
	//                                         HI   LO   TP   FP   MC
	static constexpr std::int32_t wSize[]{ 0, 128,  64,  64,  64, 256 };
	static constexpr std::int32_t hSize[]{ 0,  64,  32,  64, 128, 192 };

	const auto select{ State.schip_legacy ? 1 : std::to_underlying(mode) };
	const auto hires{ mode != Resolution::LO }; Program->screenHires = hires;
	const auto lores{ mode == Resolution::LO }; Program->screenLores = lores;

	Plane.W = wSize[select]; Plane.Wb = Plane.W - 1;
	Plane.H = hSize[select]; Plane.Hb = Plane.H - 1;
	Plane.S = Plane.W * Plane.H;

	if (State.mega_enabled) {
		Mem->foregroundBuffer.resize(true, Plane.H, Plane.W);
		Mem->backgroundBuffer.resize(true, Plane.H, Plane.W);
		Mem->collisionPalette.resize(true, Plane.H, Plane.W);
		Mem->megaPalette.resize(256);
	}
	else {
		Mem->displayBuffer[0].resize(!forced, Plane.H, Plane.W);
		if (State.xochip_color) {
			Mem->displayBuffer[1].resize(!forced, Plane.H, Plane.W);
			Mem->displayBuffer[2].resize(!forced, Plane.H, Plane.W);
			Mem->displayBuffer[3].resize(!forced, Plane.H, Plane.W);
		}
	}

	BVS->createTexture(Plane.W, Plane.H);
	BVS->setAspectRatio(
		State.mega_enabled ? 512 : 512,
		State.mega_enabled ? 384 : 256,
		State.mega_enabled ?  -2 :   2
	);

	const bool legacy{
		State.chip8E_rom   ||
		State.chip8X_rom   ||
		State.schip_legacy ||
		State.chip8_legacy
	};

	if (legacy && (forced || Quirk.waitVblank ^ lores)) {
		Program->ipf     += Program->boost *= -1;
		Quirk.waitVblank  = lores;
	}
};

void VM_Guest::loadFontData() {
	static constexpr std::uint8_t FONT_DATA[]{
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

	static constexpr std::uint8_t MEGA_FONT_DATA[]{
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
		FONT_DATA, State.schip_legacy ? 180 : 240,
		Mem->memory.begin()
	);

	if (!State.megachip_rom) return;

	std::copy_n(
		MEGA_FONT_DATA, 160,
		Mem->memory.begin() + 240
	);
}

void VM_Guest::renderToTexture() {
	BVS->lockTexture();

	if (State.mega_enabled) {
		for (auto idx{ 0 }; idx < Plane.S; ++idx) {
			BVS->pixels[idx] = Mem->foregroundBuffer.at_raw(idx);
		}
	} else if (State.xochip_color) {
		for (auto idx{ 0 }; idx < Plane.S; ++idx) {
			BVS->pixels[idx] = Color->bit[
				Mem->displayBuffer[0].at_raw(idx) << 0 |
				Mem->displayBuffer[1].at_raw(idx) << 1 |
				Mem->displayBuffer[2].at_raw(idx) << 2 |
				Mem->displayBuffer[3].at_raw(idx) << 3
			];
		}
	} else if (State.chip8X_rom) {
		for (auto idx{ 0 }; idx < Plane.S; ++idx) {
			const auto Y = idx / Plane.W & Plane.mask8X;
			const auto X = idx % Plane.W >> 3; // 8px zones

			BVS->pixels[idx] = Mem->displayBuffer[0].at_raw(idx)
				? Mem->color8xBuffer.at_raw(Y, X) : Color->bit[0];
		}
	} else {
		for (auto idx{ 0 }; idx < Plane.S; ++idx) {
			BVS->pixels[idx] = Color->bit[
				Mem->displayBuffer[0].at_raw(idx)
			];
		}
	}
	BVS->unlockTexture();
}
