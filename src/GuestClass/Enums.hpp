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
	CLEAR, // no interrupt
	FRAME, // single-frame
	SOUND, // wait for sound and stop
	DELAY, // wait for delay and proceed
	INPUT, // wait for input and proceed
	FINAL, // end state, all is well
	ERROR, // end state, error occured
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
	LO_HEX,
	HI_HEX,
	MC_HEX,
};
