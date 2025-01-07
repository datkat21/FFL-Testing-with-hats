#pragma once

static const int cMinHats = 1;
static const int cMaxHats = 10;

// temp define for now...
#define FFL_HAIR_BALD 30

typedef enum HatType
{
  HAT_TYPE_ALL = 0,
  HAT_TYPE_HAT_ONLY = 1,
  HAT_TYPE_FACE_ONLY = 2,
  HAT_TYPE_BALD = 3,
  HAT_TYPE_MAX = 4
} HatType;

static const int cHatTypes[cMaxHats] = {
    HAT_TYPE_ALL,      // 0 (off)
    HAT_TYPE_HAT_ONLY, // Cap
    HAT_TYPE_HAT_ONLY, // Beanie
    HAT_TYPE_HAT_ONLY, // Top hat
    HAT_TYPE_ALL,      // Ribbon
    HAT_TYPE_ALL,      // Bow
    HAT_TYPE_ALL,      // Cat ears
    HAT_TYPE_HAT_ONLY, // Straw hat
    HAT_TYPE_BALD,     // Hijab
    HAT_TYPE_HAT_ONLY  // Bike helmet
};