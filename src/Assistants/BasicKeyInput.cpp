
#include "BasicKeyInput.hpp"

BasicKeyInput& BasicKeyInput::create() {
    if (!_self) _self = std::make_unique<BasicKeyInput>();
    return *_self.get();
}

void BasicKeyInput::updateKeyboardCopy() {
    std::copy(
        SDL_GetKeyboardState(nullptr),
        SDL_GetKeyboardState(nullptr) + SDL_NUM_SCANCODES,
        oldKeyboardState.begin()
    );
}

bool BasicKeyInput::isKeyHeld(SDL_Scancode key, const bool prev) const noexcept {
    if (!key) return false;
    if (prev) return oldKeyboardState[key];
    else return SDL_GetKeyboardState(nullptr)[key];
}

bool BasicKeyInput::isKeyPressed(SDL_Scancode key) const noexcept {
    return !isKeyHeld(key, true) && isKeyHeld(key);
}

bool BasicKeyInput::isKeyReleased(SDL_Scancode key) const noexcept {
    return isKeyHeld(key, true) && !isKeyHeld(key);
}

std::unique_ptr<BasicKeyInput> BasicKeyInput::_self = nullptr;
std::vector<Uint8> BasicKeyInput::oldKeyboardState(SDL_NUM_SCANCODES);

BasicKeyInput& bki::kb{ BasicKeyInput::create() };
