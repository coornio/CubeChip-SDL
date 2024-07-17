/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <span>
#include <array>
#include <vector>
#include <filesystem>

#include "../Assistants/Map2D.hpp"
#include "../Types.hpp"

#include "Enums.hpp"

class MemoryBanks final {

private:
	std::vector<u8> mem{};
	u32 memIndex{};

	bool in_range(const usz pos) const noexcept { return pos < mem.size(); }
	
public:
	std::span<      u8> getSpan()       { return mem; }
	std::span<const u8> getSpan() const { return mem; }

	void resize(const usz val) { mem.resize(val); }

	std::vector<u32> megaPalette{};

	Map2D<u32> foregroundBuffer;
	Map2D<u32> backgroundBuffer;
	Map2D<u8>  collisionPalette;

	Map2D<u8>  displayBuffer[4];
	Map2D<u32> color8xBuffer;

	std::array<u8,  80> fontS;
	std::array<u8, 160> fontL;
	std::array<u8, 160> fontM;

	bool _fontDraw{};

	[[nodiscard]]
	bool fontDraw() const { return _fontDraw; }
	void fontDraw(bool state) { _fontDraw = state; }

	void modifyViewport(BrushType, s32 = 1, bool = false);

	void flushBuffers(FlushType);
	void loadPalette(s32);

	void clearPages(s32);

	auto NNNN() const { return read(counter) << 8 | read(counter + 1); }

	// Write memory at given index using given value
	void write(const usz value, const usz pos) {
		//mem[pos & mem.size() - 1] = static_cast<u8>(value);
		if (in_range(pos)) {
			mem[pos] = static_cast<u8>(value);
		}
	}
	// Write memory at saved index using given value
	void write_idx(const usz value, const usz pos = 0) {
		//mem[memIndex + pos & mem.size() - 1] = static_cast<u8>(value);
		if (in_range(memIndex + pos)) {
			mem[memIndex + pos] = static_cast<u8>(value);
		}
	}


	u8 vRegister[16]{};

public:
	auto& VX() { return vRegister[(opcode >> 8) & 0xF]; }

	//auto& regV(const usz pos)       { return vRegister[pos]; }
	//auto  regV(const usz pos) const { return vRegister[pos]; }

	void write_reg(const usz pos, const usz value) {
		vRegister[pos] = static_cast<u8>(value);
	}
	auto read_reg(const usz pos) const {
		return vRegister[pos];
	}
	void inc_reg(const usz pos, const usz value) {
		vRegister[pos] = static_cast<u8>(value);
	}

	auto index_get() const { return memIndex; }
	void index_set(const auto value) { memIndex = value; }
	void index_inc(const auto value) { memIndex += value; }
	void index_dec(const auto value) { memIndex -= value; }

	// Read memory at given index
	auto read(const usz pos) const -> u8 {
		return (in_range(pos)) ? mem[pos] : 0;
		//return mem[pos & mem.size() - 1];
	}
	// Read memory at saved index
	auto read_idx(const usz pos = 0) const -> u8 {
		return (in_range(memIndex + pos)) ? mem[memIndex + pos] : 0;
		//return mem[memIndex + pos & mem.size() - 1];
	}

private:
	std::array<u32, 16> stackBank{};
	u32 stackPtr{};

public:
	auto stackPos() const { return stackPtr; }

	s32  pageGuard{};

	u32 opcode{};
	u32 counter{};

	bool routineCall(u32);
	bool routineReturn();

	void protectPages();

	bool readPermRegs(std::filesystem::path, usz);
	bool writePermRegs(std::filesystem::path, usz);

	//u8& VX();

	void skipInstruction();
	bool jumpInstruction(u32);
	bool stepInstruction(s32);
};
