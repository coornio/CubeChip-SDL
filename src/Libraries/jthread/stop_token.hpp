
#pragma once

#include <atomic>
#include <thread>
#include <type_traits>
#include <concepts>
#include <utility>

#if defined(__x86_64__) || defined(_M_X64)
	#include <immintrin.h>
#endif

namespace nonstd {

	//-----------------------------------------------
	// internal types for shared stop state
	//-----------------------------------------------

	struct __stop_callback_base {
		void(*__callback_)(__stop_callback_base*) = nullptr;

		__stop_callback_base* __next_ = nullptr;
		__stop_callback_base** __prev_ = nullptr;
		bool* __isRemoved_ = nullptr;
		std::atomic<bool> __callbackFinishedExecuting_{ false };

		void __execute() noexcept {
			__callback_(this);
		}

	protected:
		// it shall only by us who deletes this
		// (workaround for virtual __execute() and destructor)
		~__stop_callback_base() = default;
	};

	struct __stop_state {
	public:
		void __add_token_reference() noexcept {
			__state_.fetch_add(__token_ref_increment, std::memory_order_relaxed);
		}

		void __remove_token_reference() noexcept {
			auto __oldState =
				__state_.fetch_sub(__token_ref_increment, std::memory_order_acq_rel);
			if (__oldState < (__token_ref_increment + __source_ref_increment)) {
				delete this;
			}
		}

		void __add_source_reference() noexcept {
			__state_.fetch_add(__source_ref_increment, std::memory_order_relaxed);
		}

		void __remove_source_reference() noexcept {
			auto __oldState =
				__state_.fetch_sub(__source_ref_increment, std::memory_order_acq_rel);
			if (__oldState < (__token_ref_increment + __source_ref_increment)) {
				delete this;
			}
		}

		bool __request_stop() noexcept {

			if (!__try_lock_and_signal_until_signalled()) {
				// Stop has already been requested.
				return false;
			}

			// Set the 'stop_requested' signal and acquired the lock.

			__signallingThread_ = std::this_thread::get_id();

			while (__head_ != nullptr) {
				// Dequeue the head of the queue
				auto* __cb = __head_;
				__head_ = __cb->__next_;
				const bool anyMore = __head_ != nullptr;
				if (anyMore) {
					__head_->__prev_ = &__head_;
				}
				// Mark this item as removed from the list.
				__cb->__prev_ = nullptr;

				// Don't hold lock while executing callback
				// so we don't block other threads from deregistering callbacks.
				__unlock();

				// TRICKY: Need to store a flag on the stack here that the callback
				// can use to signal that the destructor was executed inline
				// during the call. If the destructor was executed inline then
				// it's not safe to dereference __cb after __execute() returns.
				// If the destructor runs on some other thread then the other
				// thread will block waiting for this thread to signal that the
				// callback has finished executing.
				bool __isRemoved = false;
				__cb->__isRemoved_ = &__isRemoved;

				__cb->__execute();

				if (!__isRemoved) {
					__cb->__isRemoved_ = nullptr;
					__cb->__callbackFinishedExecuting_.store(
						true, std::memory_order_release);
				}

				if (!anyMore) {
					// This was the last item in the queue when we dequeued it.
					// No more items should be added to the queue after we have
					// marked the state as interrupted, only removed from the queue.
					// Avoid acquring/releasing the lock in this case.
					return true;
				}

				__lock();
			}

			__unlock();

			return true;
		}

		bool __is_stop_requested() noexcept {
			return __is_stop_requested(__state_.load(std::memory_order_acquire));
		}

		bool __is_stop_requestable() noexcept {
			return __is_stop_requestable(__state_.load(std::memory_order_acquire));
		}

		bool __try_add_callback(
			__stop_callback_base* __cb,
			bool __incrementRefCountIfSuccessful) noexcept {
			std::uint64_t __oldState;
			goto __load_state;
			do {
				goto __check_state;
				do {
					std::this_thread::yield();
				__load_state:
					__oldState = __state_.load(std::memory_order_acquire);
				__check_state:
					if (__is_stop_requested(__oldState)) {
						__cb->__execute();
						return false;
					} else if (!__is_stop_requestable(__oldState)) {
						return false;
					}
				} while (__is_locked(__oldState));
			} while (!__state_.compare_exchange_weak(
				__oldState, __oldState | __locked_flag, std::memory_order_acquire));

			// Push callback onto callback list.
			__cb->__next_ = __head_;
			if (__cb->__next_ != nullptr) {
				__cb->__next_->__prev_ = &__cb->__next_;
			}
			__cb->__prev_ = &__head_;
			__head_ = __cb;

			if (__incrementRefCountIfSuccessful) {
				__unlock_and_increment_token_ref_count();
			} else {
				__unlock();
			}

			// Successfully added the callback.
			return true;
		}

		void __remove_callback(__stop_callback_base* __cb) noexcept {
			__lock();

			if (__cb->__prev_ != nullptr) {
				// Still registered, not yet executed
				// Just remove from the list.
				*__cb->__prev_ = __cb->__next_;
				if (__cb->__next_ != nullptr) {
					__cb->__next_->__prev_ = __cb->__prev_;
				}

				__unlock_and_decrement_token_ref_count();

				return;
			}

			__unlock();

			// Callback has either already executed or is executing
			// concurrently on another thread.

			if (__signallingThread_ == std::this_thread::get_id()) {
				// Callback executed on this thread or is still currently executing
				// and is deregistering itself from within the callback.
				if (__cb->__isRemoved_ != nullptr) {
					// Currently inside the callback, let the __request_stop() method
					// know the object is about to be destructed and that it should
					// not try to access the object when the callback returns.
					*__cb->__isRemoved_ = true;
				}
			} else {
				// Callback is currently executing on another thread,
				// block until it finishes executing.
				while (
					!__cb->__callbackFinishedExecuting_.load(std::memory_order_acquire)) {
					std::this_thread::yield();
				}
			}

			__remove_token_reference();
		}

	private:
		static bool __is_locked(std::uint64_t __state) noexcept {
			return (__state & __locked_flag) != 0;
		}

		static bool __is_stop_requested(std::uint64_t __state) noexcept {
			return (__state & __stop_requested_flag) != 0;
		}

		static bool __is_stop_requestable(std::uint64_t __state) noexcept {
			// Interruptible if it has already been interrupted or if there are
			// still interrupt_source instances in existence.
			return __is_stop_requested(__state) || (__state >= __source_ref_increment);
		}

		bool __try_lock_and_signal_until_signalled() noexcept {
			std::uint64_t __oldState = __state_.load(std::memory_order_acquire);
			do {
				if (__is_stop_requested(__oldState))
					return false;
				while (__is_locked(__oldState)) {
					std::this_thread::yield();
					__oldState = __state_.load(std::memory_order_acquire);
					if (__is_stop_requested(__oldState))
						return false;
				}
			} while (!__state_.compare_exchange_weak(
				__oldState,
				__oldState | __stop_requested_flag | __locked_flag,
				std::memory_order_acq_rel,
				std::memory_order_acquire));
			return true;
		}

		void __lock() noexcept {
			auto __oldState = __state_.load(std::memory_order_relaxed);
			do {
				while (__is_locked(__oldState)) {
					std::this_thread::yield();
					__oldState = __state_.load(std::memory_order_relaxed);
				}
			} while (!__state_.compare_exchange_weak(
				__oldState,
				__oldState | __locked_flag,
				std::memory_order_acquire,
				std::memory_order_relaxed));
		}

		void __unlock() noexcept {
			__state_.fetch_sub(__locked_flag, std::memory_order_release);
		}

		void __unlock_and_increment_token_ref_count() noexcept {
			__state_.fetch_sub(
				__locked_flag - __token_ref_increment, std::memory_order_release);
		}

		void __unlock_and_decrement_token_ref_count() noexcept {
			auto __oldState = __state_.fetch_sub(
				__locked_flag + __token_ref_increment, std::memory_order_acq_rel);
			// Check if new state is less than __token_ref_increment which would
			// indicate that this was the last reference.
			if (__oldState <
				(__locked_flag + __token_ref_increment + __token_ref_increment)) {
				delete this;
			}
		}

		static constexpr std::uint64_t __stop_requested_flag = 1u;
		static constexpr std::uint64_t __locked_flag = 2u;
		static constexpr std::uint64_t __token_ref_increment = 4u;
		static constexpr std::uint64_t __source_ref_increment =
			static_cast<std::uint64_t>(1u) << 33u;

		// bit 0 - stop-requested
		// bit 1 - locked
		// bits 2-32 - token ref count (31 bits)
		// bits 33-63 - source ref count (31 bits)
		std::atomic<std::uint64_t> __state_{ __source_ref_increment };
		__stop_callback_base* __head_ = nullptr;
		std::thread::id __signallingThread_{};
	};

	//-----------------------------------------------
	// forward declarations
	//-----------------------------------------------

	class stop_source;
	template <typename _Callback>
	class stop_callback;

	// std::nostopstate
	// - to initialize a stop_source without shared stop state
	struct nostopstate_t { explicit nostopstate_t() = default; };
	inline constexpr nostopstate_t nostopstate{};

	//-----------------------------------------------
	// stop_token
	//-----------------------------------------------

	class stop_token {
		friend class stop_source;
		template <typename _Callback>
		friend class stop_callback;

		explicit stop_token(__stop_state* state) noexcept
			: pStopState(state)
		{
			if (pStopState != nullptr) {
				pStopState->__add_token_reference();
			}
		}

		__stop_state* pStopState;

	public:
		// construct:
		// - TODO: explicit?
		stop_token() noexcept
			: pStopState(nullptr)
		{}

		// copy/move/assign/destroy:
		stop_token(const stop_token& other) noexcept
			: pStopState(other.pStopState) {
			if (pStopState != nullptr) {
				pStopState->__add_token_reference();
			}
		}

		stop_token(stop_token&& other) noexcept
			: pStopState(std::exchange(other.pStopState, nullptr)) {}

		~stop_token() {
			if (pStopState != nullptr) {
				pStopState->__remove_token_reference();
			}
		}

		stop_token& operator=(const stop_token& other) noexcept {
			if (pStopState != other.pStopState) {
				stop_token __tmp{ other };
				swap(__tmp);
			}
			return *this;
		}

		stop_token& operator=(stop_token&& other) noexcept {
			stop_token __tmp{ std::move(other) };
			swap(__tmp);
			return *this;
		}

		void swap(stop_token& other) noexcept {
			std::swap(pStopState, other.pStopState);
		}

		// stop handling:
		[[nodiscard]] bool stop_requested() const noexcept {
			return pStopState != nullptr && pStopState->__is_stop_requested();
		}

		[[nodiscard]] bool stop_possible() const noexcept {
			return pStopState != nullptr && pStopState->__is_stop_requestable();
		}

		[[nodiscard]] friend bool operator==(
			const stop_token& lhs,
			const stop_token& rhs) noexcept {
			return lhs.pStopState == rhs.pStopState;
		}
		[[nodiscard]] friend bool operator!=(
			const stop_token& lhs,
			const stop_token& rhs) noexcept {
			return lhs.pStopState != rhs.pStopState;
		}
	};

	//-----------------------------------------------
	// stop_source
	//-----------------------------------------------

	class stop_source {
		__stop_state* pStopState;

	public:
		stop_source()
			: pStopState(new __stop_state())
		{}

		explicit stop_source(nostopstate_t) noexcept
			: pStopState(nullptr)
		{}

		~stop_source() {
			if (pStopState != nullptr) {
				pStopState->__remove_source_reference();
			}
		}

		stop_source(const stop_source& other) noexcept
			: pStopState(other.pStopState)
		{
			if (pStopState != nullptr) {
				pStopState->__add_source_reference();
			}
		}

		stop_source(stop_source&& other) noexcept
			: pStopState(std::exchange(other.pStopState, nullptr))
		{}

		stop_source& operator=(stop_source&& other) noexcept {
			stop_source __tmp{ std::move(other) };
			swap(__tmp);
			return *this;
		}

		stop_source& operator=(const stop_source& other) noexcept {
			if (pStopState != other.pStopState) {
				stop_source __tmp{ other };
				swap(__tmp);
			}
			return *this;
		}

		[[nodiscard]] bool stop_requested() const noexcept {
			return pStopState != nullptr && pStopState->__is_stop_requested();
		}

		[[nodiscard]] bool stop_possible() const noexcept {
			return pStopState != nullptr;
		}

		bool request_stop() noexcept {
			if (pStopState != nullptr) {
				return pStopState->__request_stop();
			}
			return false;
		}

		[[nodiscard]] stop_token get_token() const noexcept {
			return stop_token{ pStopState };
		}

		void swap(stop_source& other) noexcept {
			std::swap(pStopState, other.pStopState);
		}

		[[nodiscard]] friend bool operator==(
			const stop_source& lhs,
			const stop_source& rhs) noexcept {
			return lhs.pStopState == rhs.pStopState;
		}
		[[nodiscard]] friend bool operator!=(
			const stop_source& lhs,
			const stop_source& rhs) noexcept {
			return lhs.pStopState != rhs.pStopState;
		}
	};

	//-----------------------------------------------
	// stop_callback
	//-----------------------------------------------

	template <typename Callback>
	// requires Destructible<_Callback> && Invocable<_Callback>
	class [[nodiscard]] stop_callback : private __stop_callback_base {
		void __execute() noexcept {
			// Executed in a noexcept context
			// If it throws then we call std::terminate().
			mCallback();
		}

		__stop_state* pStopState;
		Callback mCallback;

	public:
		using callback_type = Callback;

		template <typename _CB>
			requires (std::is_constructible_v<Callback, _CB>)
		explicit stop_callback(const stop_token& token, _CB&& callback) noexcept(std::is_nothrow_constructible_v<Callback, _CB>)
			: __stop_callback_base{ [](__stop_callback_base* that) noexcept {
				static_cast<stop_callback*>(that)->__execute();
			} }
			, pStopState(nullptr)
			, mCallback(static_cast<_CB&&>(callback))
		{
			if (token.pStopState && token.pStopState->__try_add_callback(this, true)) {
				pStopState = token.pStopState;
			}
		}

		template <typename _CB>
			requires (std::is_constructible_v<Callback, _CB>)
		explicit stop_callback(stop_token&& token, _CB&& callback) noexcept(std::is_nothrow_constructible_v<Callback, _CB>)
			: __stop_callback_base{ [](__stop_callback_base* that) noexcept {
				static_cast<stop_callback*>(that)->__execute();
			} }
			, pStopState(nullptr)
			, mCallback(static_cast<_CB&&>(callback))
		{
			if (token.pStopState && token.pStopState->__try_add_callback(this, false)) {
				pStopState = std::exchange(token.pStopState, nullptr);
			}
		}

		~stop_callback() {
			if (pStopState) {
				pStopState->__remove_callback(this);
			}
		}

		stop_callback& operator=(const stop_callback&) = delete;
		stop_callback& operator=(stop_callback&&) = delete;
		stop_callback(const stop_callback&) = delete;
		stop_callback(stop_callback&&) = delete;
	};

	template <typename _Callback>
	stop_callback(stop_token, _Callback) -> stop_callback<_Callback>;
}
