#pragma once

#include <xkbcommon/xkbcommon.h>

namespace qs::wayland::input_method::impl {
enum class DirectionKey : uint8_t { UP, DOWN, LEFT, RIGHT };

inline constexpr xkb_keycode_t WAYLAND_KEY_OFFSET = 8;
} // namespace qs::wayland::input_method::impl
