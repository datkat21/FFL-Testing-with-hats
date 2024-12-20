// idk
static const int cMaxHats = 10;

// const int HATS_USING_ALL[] = {4, 5, 6};
// const int HATS_USING_HAT_ONLY[] = {1, 2, 3, 7, 9};
// const int HATS_USING_FACE_ONLY[] = {8};

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

static const int cHatTypes[MAX_HATS] = {
    // Ignore 0
    HAT_TYPE_ALL,
    // Cap
    HAT_TYPE_HAT_ONLY,
    // Beanie
    HAT_TYPE_HAT_ONLY,
    // Top hat
    HAT_TYPE_HAT_ONLY,
    // Ribbon
    HAT_TYPE_ALL,
    // Bow
    HAT_TYPE_ALL,
    // Cat ears
    HAT_TYPE_ALL,
    // Straw hat
    HAT_TYPE_HAT_ONLY,
    // Hijab
    HAT_TYPE_BALD,
    // Bike helmet
    HAT_TYPE_HAT_ONLY};