/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

enum class Resolution {
	ERROR,
	HI, // 128 x  64 - 2:1
	LO, //  64 x  32 - 2:1
	TP, //  64 x  64 - 2:1
	FP, //  64 x 128 - 2:1
	MC, // 256 x 192 - 4:3
};

enum class Interrupt {
	NONE,
	ONCE,
	STOP,
	WAIT,
	FX0A,
	HALT,
};

enum class BrushType {
	CLR,
	XOR,
	SUB,
	ADD,
};

enum class FlushType {
	DISPLAY,
	DISCARD,
};

enum class Index {
	MEMORY,
	FONT_S,
	FONT_L,
	FONT_M,
};
