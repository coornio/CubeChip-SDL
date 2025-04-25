/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL3/SDL.h>

#include <memory>

/*==================================================================*/
	#pragma region SDL_Deleter & Specializations

template <typename T>
struct SDL_Deleter {
	using type = std::default_delete<T>;
};
template <>
struct SDL_Deleter<SDL_Window> {
	static void deleter(SDL_Window* ptr) { if (ptr) { SDL_DestroyWindow(ptr); } }
	using type = decltype(&deleter);
};
template <>
struct SDL_Deleter<SDL_Renderer> {
	static void deleter(SDL_Renderer* ptr) { if (ptr) { SDL_DestroyRenderer(ptr); } }
	using type = decltype(&deleter);
};
template <>
struct SDL_Deleter<SDL_Texture> {
	static void deleter(SDL_Texture* ptr) { if (ptr) { SDL_DestroyTexture(ptr); } }
	using type = decltype(&deleter);
};
template <>
struct SDL_Deleter<SDL_AudioStream> {
	static void deleter(SDL_AudioStream* ptr) { if (ptr) { SDL_DestroyAudioStream(ptr); } }
	using type = decltype(&deleter);
};
template <>
struct SDL_Deleter<SDL_DisplayID> {
	static void deleter(SDL_DisplayID* ptr) { if (ptr) { SDL_free(ptr); } }
	using type = decltype(&deleter);
};
template <>
struct SDL_Deleter<char> {
	static void deleter(char* ptr) { if (ptr) { SDL_free(ptr); } }
	using type = decltype(&deleter);
};
template <>
struct SDL_Deleter<const char> {
	static void deleter(const char* ptr) { if (ptr) { SDL_free(const_cast<char*>(ptr)); } }
	using type = decltype(&deleter);
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region SDL_Unique

template <typename T>
class SDL_Unique {
	using DeleterType = typename SDL_Deleter<T>::type;
	std::unique_ptr<T, DeleterType> mPtr;

public:
	SDL_Unique(T* ptr = nullptr) noexcept
		: mPtr{ ptr, SDL_Deleter<T>::deleter }
	{}

	SDL_Unique(SDL_Unique&&) noexcept = default; // move constructor
	SDL_Unique(const SDL_Unique&) = delete; // copy constructor

	SDL_Unique& operator=(SDL_Unique&&) noexcept = default; // move assignment
	SDL_Unique& operator=(const SDL_Unique&) = delete; // copy assignment

	SDL_Unique& operator=(T* ptr) noexcept { reset(ptr); return *this; }

	void reset(T* ptr = nullptr) noexcept { mPtr.reset(ptr); }
	T* release()                 noexcept { return mPtr.release(); }
	T* get()               const noexcept { return mPtr.get(); }

	operator T*()   const noexcept { return mPtr.get(); }
	operator bool() const noexcept { return static_cast<bool>(mPtr); }
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
