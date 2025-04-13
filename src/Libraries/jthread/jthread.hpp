// -----------------------------------------------------
// cooperative interruptable and joining thread:
// -----------------------------------------------------

#pragma once

#include "stop_token.hpp"

#include <thread>
#include <type_traits>
#include <functional>

namespace nonstd {

	class jthread {
		stop_source mStopSource;
		std::thread mThread{};

	public:
		using id = std::thread::id;
		using native_handle_type = std::thread::native_handle_type;

		jthread() noexcept
			: mStopSource{ nostopstate }
		{}

		template <typename Callable, typename... Args>
			requires (!std::is_same_v<std::decay_t<Callable>, jthread>)
		explicit jthread(Callable&& func, Args&&... args)
			: mStopSource{}
			, mThread{ [](stop_token token, auto&& func, auto&&... args) {
				if constexpr (std::is_invocable_v<Callable, stop_token, Args...>) {
					std::invoke(std::forward<decltype(func)>(func),
								std::move(token),
								std::forward<decltype(args)>(args)...);
					}
				else {
					std::invoke(std::forward<decltype(func)>(func),
								std::forward<decltype(args)>(args)...);
					}
				}
				, mStopSource.get_token()      // not captured due to possible races if immediately set
				, std::forward<Callable>(func) // pass callable
				, std::forward<Args>(args)...  // pass arguments for callable
			}
		{}

		~jthread() {
			if (joinable()) {
				request_stop();
				join();
			}
		}

		jthread(const jthread&) = delete;
		jthread(jthread&&) noexcept = default;
		jthread& operator=(const jthread&) = delete;
		jthread& operator=(jthread&& other) noexcept {
			if (joinable()) {
				request_stop();
				join();
			}

			mThread     = std::move(other.mThread);
			mStopSource = std::move(other.mStopSource);
			return *this;
		}


		void swap(jthread& other) noexcept {
			std::swap(mStopSource, other.mStopSource);
			std::swap(mThread, other.mThread);
		}
		bool joinable() const noexcept {
			return mThread.joinable();
		}
		void join() {
			mThread.join();
		}
		void detach() {
			mThread.detach();
		}

		id get_id() const noexcept {
			return mThread.get_id();
		}
		native_handle_type native_handle() noexcept {
			return mThread.native_handle();
		}


		static unsigned hardware_concurrency() noexcept {
			return std::thread::hardware_concurrency();
		};


		[[nodiscard]]
		auto get_stop_source() noexcept {
			return mStopSource;
		}
		[[nodiscard]]
		auto get_stop_token() const noexcept {
			return mStopSource.get_token();
		}
		bool request_stop() noexcept {
			return get_stop_source().request_stop();
		}
	};

}
