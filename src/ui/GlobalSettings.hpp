#ifndef MMM_GLOBALSETTINGS_HPP
#define MMM_GLOBALSETTINGS_HPP

#include <glm/glm.hpp>

enum class GlobalTheme : uint32_t {
    OPEN_LIGHT = 0,
    OPEN_DARK = 1,
    COLIN_DARK = 2,
    COLIN_LIGHT = 3,
};

struct Settings {
    glm::vec2 window_pos;
    GlobalTheme theme;
};

#endif  // MMM_GLOBALSETTINGS_HPP
