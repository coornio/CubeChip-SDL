/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../BytePusher_CoreInterface.hpp"

/*==================================================================*/

class BYTEPUSHER_STANDARD final : public BytePusher_CoreInterface {
	static constexpr u32 cTotalMemory{ ::CalcBytes(16, MiB) };
	static constexpr u32 cSafezoneOOB{         8 };
	static constexpr f32 cRefreshRate{    60.00f };
	static constexpr s32 cAudioLength{       256 };
	static constexpr s32 cScreenSizeX{       256 };
	static constexpr s32 cScreenSizeY{       256 };
	static constexpr s32 cScreenSizeT{ 256 * 256 };

private:
	std::array<u8, cTotalMemory + cSafezoneOOB>
		mMemoryBank{};

	template<u32 T> requires (T >= 1 && T <= 3)
		u32 readData(u32 pos) const noexcept {
		if        constexpr (T == 1) {
			return mMemoryBank[pos + 0];
		} else if constexpr (T == 2) {
			return mMemoryBank[pos + 0] << 8
				 | mMemoryBank[pos + 1];
		} else if constexpr (T == 3) {
			return mMemoryBank[pos + 0] << 16
				 | mMemoryBank[pos + 1] << 8
				 | mMemoryBank[pos + 2];
		}
	}

	void instructionLoop() noexcept override;
	void renderAudioData() override;
	void renderVideoData() override;

public:
	BYTEPUSHER_STANDARD();

	static constexpr bool isGameFileValid(
		const char* fileData,
		const ust   fileSize
	) noexcept {
		if (!fileData || !fileSize) { return false; }
		return fileSize <= cTotalMemory;
	}
};
