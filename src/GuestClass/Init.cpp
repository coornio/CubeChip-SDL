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
	switch (File->hash) {

		case (FileTypes::c2x):
			if (!loadRomToRam(4'096, 0x300))
				return false;
			Program->init(0x300, 30);
			Program->setFncSet(&SetClassic8);
			State.chip8X_rom   = true;
			State.hires_2paged = true;
			break;

		case (FileTypes::c4x):
			if (!loadRomToRam(4'096, 0x300))
				return false;
			Program->init(0x300, 30);
			Program->setFncSet(&SetClassic8);
			State.chip8X_rom   = true;
			State.hires_2paged = true;
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
			Program->init(0x200, 11);
			Program->setFncSet(&SetClassic8);
			State.chip8E_rom = true;
			break;

		case (FileTypes::c2h):
			if (!loadRomToRam(4'096, 0x260))
				return false;
			Program->init(0x260, 30);
			Program->setFncSet(&SetClassic8);
			State.chip_classic = true;
			State.hires_2paged = true;
			break;

		case (FileTypes::c4h):
			if (!loadRomToRam(4'096, 0x244))
				return false;
			Program->init(0x244, 30);
			Program->setFncSet(&SetClassic8);
			State.chip_classic = true;
			State.hires_2paged = true;
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
			Program->init(0x200, 2'800'000);
			Program->setFncSet(&SetClassic8);
			State.chip_classic = true;
			blog.stdLogOut("Unknown rom type, default parameters apply.");
	}
	return true;
}

bool VM_Guest::loadRomToRam(const std::size_t size, const std::size_t offset) {
	Program->limiter = size - 1; // set program memory limit
	Mem->ram.resize(size);       // resize the memory vector

	std::basic_ifstream<char> ifs(File->path, std::ios::binary);
	ifs.read(reinterpret_cast<char*>(&Mem->ram[offset]), File->size);
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
	Program->setSpeed(0);
	//Quirk.waitScroll = true;
	//Quirk.waitVblank = true;
	//Quirk.wrapSprite = true;
	Input->reset();

	// XXX - apply custom rom settings here

	if (State.hires_2paged || State.hires_4paged) {
		State.schip_legacy = false; // incompatible together
	}
	if (State.megachip_rom) {
		State.schip_legacy = false; // incompatible together
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
	if (State.chip8X_rom) {
		Color->cycleBackground();

		if (!State.schip_legacy)
			Mem->bufColor8x[0][0] = Color->getFore8X(2);
		else {
			Mem->bufColor8x[0][0] =
			Mem->bufColor8x[0][1] =
			Mem->bufColor8x[4][0] =
			Mem->bufColor8x[4][1] = Color->getFore8X(2);
		}
	}

	setupDisplay(Resolution::LO + State.hires_2paged + State.hires_4paged, true);
}

void VM_Guest::setupDisplay(const std::int32_t mode, const bool forced) {
	//                                 HI   LO   TP   FP   MC
	static constexpr int32_t wSize[]{ 128,  64,  64,  64, 256 };
	static constexpr int32_t hSize[]{  64,  32,  64, 128, 192 };

	const auto modeSelect{ State.schip_legacy ? 0 : mode - 1 };

	Plane.W = wSize[modeSelect]; Plane.Wb = Plane.W - 1;
	Plane.H = hSize[modeSelect]; Plane.Hb = Plane.H - 1;
	Plane.X = Plane.W >> 3;      Plane.Xb = Plane.X - 1;

	Program->screenMode = mode;

	if (State.mega_enabled) {
		Mem->display.resize(!forced, Plane.H, Plane.W);
		Mem->bufColorMC.resize(true, Plane.H, Plane.W);
		Mem->bufPalette.resize(true, Plane.H, Plane.W);
	} else {
		Mem->display.resize(!forced, Plane.H, Plane.X);
		Mem->bufColorMC.resize(true, 1, 1);
		Mem->bufPalette.resize(true, 1, 1);
	}
	if (State.chip8X_rom) {
		Mem->bufColor8x.resize(false, Plane.H, Plane.X);
	}

	Video->createTexture(Plane.W, Plane.H);
	Video->setTextureBlend(SDL_BLENDMODE_BLEND);
	Video->setAspectRatio(
		State.mega_enabled ? 256 : 512,
		State.mega_enabled ? 192 : 256,
		State.mega_enabled ?  -2 :   4
	);

	isDisplayReady(true);

	const bool legacy{ State.chip8X_rom || State.schip_legacy || State.chip8_legacy };
	const bool lores{ Program->screenMode == Resolution::LO };

	if (legacy && (forced || Quirk.waitVblank ^ lores)) {
		Program->ipf += Program->boost *= -1;
		Quirk.waitVblank = lores;
	}
};

void VM_Guest::loadFontData() {
	static constexpr std::array<uint8_t, 80 + 160> FONT_DATA{
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

	static constexpr std::array<uint8_t, 160> MEGA_FONT_DATA{
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
	std::copy(
		FONT_DATA.begin(),
		FONT_DATA.end() - (State.schip_legacy ? 60 : 0),
		Mem->ram.begin()
	);

	if (!State.megachip_rom) return;
	if (!State.gigachip_rom) return;

	std::copy(
		MEGA_FONT_DATA.begin(),
		MEGA_FONT_DATA.end(),
		Mem->ram.begin() + 240
	);
}

void VM_Guest::flushDisplay() {
	auto*& pixels{ Video->pixels };
	Video->lockTexture();

	if (State.mega_enabled) {
		for (const auto& row : Mem->display) {
			for (const auto& elem : row) {
				*pixels++ = elem;
			}
		}
	} else if (State.xochip_color) {
		for (const auto& row : Mem->display) {
			for (const auto& elem : row) {
				for (auto B{ 28 }; B >= 0; B -= 4) {
					*pixels++ = Color->bit[elem >> B & 0xF];
				}
			}
		}
	} else if (State.chip8X_rom) {
		const auto mask{ State.chip8X_hires ? 0xFF : 0xFC };
		for (auto Y{ 0 }; const auto & row : Mem->display) {
			for (auto X{ 0 }; const auto & elem : row) {
				const auto color{ Mem->bufColor8x[Y & mask][X] };
				for (auto B{ 7 }; B >= 0; --B) {
					*pixels++ = (elem >> B & 0x1) ? color : Color->bit[0];
				}
				++X;
			}
			++Y;
		}
	} else {
		for (const auto& row : Mem->display) {
			for (const auto& elem : row) {
				for (auto B{ 7 }; B >= 0; --B) {
					*pixels++ = Color->bit[elem >> B & 1];
				}
			}
		}
	}
	Video->unlockTexture();
}
