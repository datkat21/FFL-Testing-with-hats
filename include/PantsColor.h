#include <nn/ffl.h>

enum PantsColor {
    PANTS_COLOR_GRAY = 0,
    PANTS_COLOR_BLUE,
    PANTS_COLOR_RED,
    PANTS_COLOR_GOLD,
    PANTS_COLOR_MAX
};

const FFLColor cPantsColors[PANTS_COLOR_MAX] =
{
    // gray - one of your own miis
    { 0.25098f, 0.27451f, 0.30588f, 1.0f },

    // red - favorite/account mii
    { 0.43922f, 0.12549f, 0.06275f, 1.0f },

    // blue - foreign mii from other console
    { 0.15686f, 0.25098f, 0.47059f, 1.0f },

    // gold - special mii created by N
    { 0.75294f, 0.62745f, 0.18824f, 1.0f }
};
