// -----------------------------------------------------
// cooperative interruptable and joining thread:
// -----------------------------------------------------

#pragma once

#include "stop_token.hpp"
#include <thread>
#include <future>
#include <type_traits>
#include <functional> // for invoke()

namespace nonstd {

	//*****************************************
	//* class jthread
	//* - joining std::thread with signaling stop/end support
	//*****************************************
	class jthread {
		//*** API for the starting thread:
		stop_source mStopSource; // stop_source for started thread
		std::thread mThread{};   // started thread (if any)

	public:
		//*****************************************
		//* standardized API:
		//*****************************************
		// - cover full API of std::thread
		//   to be able to switch from std::thread to std::jthread

		// types are those from std::thread:
		using id = std::thread::id;
		using native_handle_type = std::thread::native_handle_type;

		// construct/copy/destroy:
		jthread() noexcept;
		// template <typename F, typename... Args> explicit jthread(F&& f, Args&&... args);
		// THE constructor that starts the thread:
		// - NOTE: does SFINAE out copy constructor semantics
		template <typename Callable, typename... Args,
			typename = std::enable_if_t<!std::is_same_v<std::decay_t<Callable>, jthread>>>
		explicit jthread(Callable&& cb, Args&&... args);
		~jthread();

		jthread(const jthread&) = delete;
		jthread(jthread&&) noexcept = default;
		jthread& operator=(const jthread&) = delete;
		jthread& operator=(jthread&&) noexcept;

		// members:
		void swap(jthread&) noexcept;
		bool joinable() const noexcept;
		void join();
		void detach();

		id get_id() const noexcept;
		native_handle_type native_handle();

		// static members:
		static unsigned hardware_concurrency() noexcept {
			return std::thread::hardware_concurrency();
		};

		//***************************************** 
		// - supplementary API:
		//   - for the calling thread:
		[[nodiscard]] stop_source get_stop_source() noexcept;
		[[nodiscard]] stop_token get_stop_token() const noexcept;
		bool request_stop() noexcept {
			return get_stop_source().request_stop();
		}
	};

//**********************************************************************

	//***************************************** 
	//* implementation of class jthread
	//***************************************** 

	// default constructor:
	inline jthread::jthread() noexcept
		: mStopSource{ nostopstate }
	{}

	// THE constructor that starts the thread:
	// - NOTE: declaration does SFINAE out copy constructor semantics
	template <typename Callable, typename... Args, typename>
	inline jthread::jthread(Callable&& cb, Args&&... args)
		: mStopSource{}                                           // initialize stop_source
		, mThread{ [](stop_token st, auto&& cb, auto&&... args) { // called lambda in the thread
			// perform tasks of the thread:
			if constexpr(std::is_invocable_v<Callable, stop_token, Args...>) {
				// pass the stop_token as first argument to the started thread:
				std::invoke(std::forward<decltype(cb)>(cb),
							std::move(st),
							std::forward<decltype(args)>(args)...);
				}
			else {
				// started thread does not expect a stop token:
				std::invoke(std::forward<decltype(cb)>(cb),
							std::forward<decltype(args)>(args)...);
				}
			},
			mStopSource.get_token(),     // not captured due to possible races if immediately set
			std::forward<Callable>(cb),  // pass callable
			std::forward<Args>(args)...  // pass arguments for callable
		}
	{}

	// move assignment operator:
	inline jthread& jthread::operator=(jthread&& other) noexcept {
		if (joinable()) { // if not joined/detached, signal stop and wait for end:
			request_stop();
			join();
		}

		mThread = std::move(other.mThread);
		mStopSource = std::move(other.mStopSource);
		return *this;
	}

	// destructor:
	inline jthread::~jthread() {
		if (joinable()) {   // if not joined/detached, signal stop and wait for end:
			request_stop();
			join();
		}
	}

	// others:
	inline bool jthread::joinable() const noexcept {
		return mThread.joinable();
	}
	inline void jthread::join() {
		mThread.join();
	}
	inline void jthread::detach() {
		mThread.detach();
	}
	inline typename jthread::id jthread::get_id() const noexcept {
		return mThread.get_id();
	}
	inline typename jthread::native_handle_type jthread::native_handle() {
		return mThread.native_handle();
	}

	inline stop_source jthread::get_stop_source() noexcept {
		return mStopSource;
	}
	inline stop_token jthread::get_stop_token() const noexcept {
		return mStopSource.get_token();
	}

	inline void jthread::swap(jthread& other) noexcept {
		std::swap(mStopSource, other.mStopSource);
		std::swap(mThread, other.mThread);
	}
}
