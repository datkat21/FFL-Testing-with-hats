#pragma once

// NOTE: from libnx
typedef struct { u8 uuid[0x10]; } Uuid;   ///< Unique identifier.

// NOTE: below struct is from libnx nx/include/switch/services/mii.h#L57
// Mii create ID.
typedef struct {
    Uuid uuid;
} MiiCreateId;


typedef struct {  /* charInfo */
    MiiCreateId create_id;
    char16_t nickname[11]; /* null terminated */
    u8 font_region;
    u8 favorite_color;
    u8 gender;
    u8 height;
    u8 build;
    u8 type; /* special or not */
    u8 region_move;
    u8 faceline_type;
    u8 faceline_color;
    u8 faceline_wrinkle;
    u8 faceline_make;
    u8 hair_type;
    u8 hair_color;
    u8 hair_flip;
    u8 eye_type;
    u8 eye_color;
    u8 eye_scale;
    u8 eye_aspect;
    u8 eye_rotate;
    u8 eye_x;
    u8 eye_y;
    u8 eyebrow_type;
    u8 eyebrow_color;
    u8 eyebrow_scale;
    u8 eyebrow_aspect;
    u8 eyebrow_rotate;
    u8 eyebrow_x;
    u8 eyebrow_y;
    u8 nose_type;
    u8 nose_scale;
    u8 nose_y;
    u8 mouth_type;
    u8 mouth_color;
    u8 mouth_scale;
    u8 mouth_aspect;
    u8 mouth_y;
    u8 beard_color;
    u8 beard_type;
    u8 mustache_type;
    u8 mustache_scale;
    u8 mustache_y;
    u8 glass_type;
    u8 glass_color;
    u8 glass_scale;
    u8 glass_y;
    u8 mole_type;
    u8 mole_scale;
    u8 mole_x;
    u8 mole_y;
    u8 reserved; /* always zero */
} charInfo;

typedef struct {  /* coreData */
    u8 hair_type;

    u8 height:7;
    u8 mole_type:1;

    u8 build:7;
    u8 hair_flip:1;

    u8 hair_color:7;
    u8 type:1;

    u8 eye_color:7;
    u8 gender:1;

    u8 eyebrow_color:7;
    u8 unused1:1;

    u8 mouth_color:7;
    u8 unused2:1;

    u8 beard_color:7;
    u8 unused3:1;

    u8 glass_color:7;
    u8 unused4:1;

    u8 eye_type:6;
    u8 region_move:2;

    u8 mouth_type:6;
    u8 font_region:2;

    u8 eye_y:5;
    u8 glass_scale:3;

    u8 eyebrow_type:5;
    u8 mustache_type:3;

    u8 nose_type:5;
    u8 beard_type:3;

    u8 nose_y:5;
    u8 mouth_aspect:3;

    u8 mouth_y:5;
    u8 eyebrow_aspect:3;

    u8 mustache_y:5;
    u8 eye_rotate:3;

    u8 glass_y:5;
    u8 eye_aspect:3;

    u8 mole_x:5;
    u8 eye_scale:3;

    u8 mole_y:5;
    u8 unused5:3;

    u8 glass_type:5;
    u8 unused6:3;

    u8 favorite_color:4;
    u8 faceline_type:4;

    u8 faceline_color:4;
    u8 faceline_wrinkle:4;

    u8 faceline_make:4;
    u8 eye_x:4;

    u8 eyebrow_scale:4;
    u8 eyebrow_rotate:4;

    u8 eyebrow_x:4;
    u8 eyebrow_y:4; /* value + 3 = true value */

    u8 nose_scale:4;
    u8 mouth_scale:4;

    u8 mustache_scale:4;
    u8 mole_scale:4;

    char16_t nickname[10]; /* Not null terminated */
} coreData;

typedef struct {
    coreData core_data;
    MiiCreateId create_id;
    u16 crc16;
    u16 crc16_device;
} storeData;

typedef struct {
    char magic[4]; /* NFDB */
    storeData entries[100];
    u8 version; /* always 1 */
    u8 entry_count;
    u16 crc16;
} NFDB;

typedef struct {
    char magic[4]; /* NFIF */
    coreData entries[100];
    u8 version; /* always 2 */
    u8 entry_count;
    u8 unused[2];
    u8 hmac_sha256[0x20]; /* key found in sdb main. Look at database export function */
} NFIF;

// NOTE: not in miiport
typedef struct {
    u8 beard_color;       // Beard color
    u8 beard_type;        // Beard (goatee) type
    u8 build;             // Body weight
    u8 eye_aspect;        // Eye stretch
    u8 eye_color;         // Eye color
    u8 eye_rotate;        // Eye rotation
    u8 eye_scale;         // Eye size
    u8 eye_type;          // Eye type
    u8 eye_x;             // Eye X (horizontal) distance
    u8 eye_y;             // Eye Y (vertical) position
    u8 eyebrow_aspect;    // Eyebrow stretch
    u8 eyebrow_color;     // Eyebrow color
    u8 eyebrow_rotate;    // Eyebrow rotation
    u8 eyebrow_scale;     // Eyebrow size
    u8 eyebrow_type;      // Eyebrow type
    u8 eyebrow_x;         // Eyebrow X (horizontal) distance
    u8 eyebrow_y;         // Eyebrow Y (vertical) distance
    u8 faceline_color;    // Skin color
    u8 faceline_make;     // Face makeup
    u8 faceline_type;     // Face shape
    u8 faceline_wrinkle;  // Face wrinkles
    u8 favorite_color;    // Favorite color
    u8 gender;            // Mii gender
    u8 glass_color;       // Glasses color
    u8 glass_scale;       // Glasses size
    u8 glass_type;        // Glasses type
    u8 glass_y;           // Glasses Y (vertical) position
    u8 hair_color;        // Hair color
    u8 hair_flip;         // Flip hair
    u8 hair_type;         // Hair type
    u8 height;            // Body height
    u8 mole_scale;        // Beauty mark size
    u8 mole_type;         // Enable beauty mark
    u8 mole_x;            // Beauty mark X (horizontal) position
    u8 mole_y;            // Beauty mark Y (vertical) position
    u8 mouth_aspect;      // Mouth stretch
    u8 mouth_color;       // Mouth color
    u8 mouth_scale;       // Mouth size
    u8 mouth_type;        // Mouth type
    u8 mouth_y;           // Mouth Y (vertical) position
    u8 mustache_scale;    // Mustache size
    u8 mustache_type;     // Mustache type
    u8 mustache_y;        // Mustache Y (vertical) position
    u8 nose_scale;        // Nose size
    u8 nose_type;         // Nose type
    u8 nose_y;            // Nose Y (vertical) position
} charInfoStudio;

#define STUDIO_DATA_ENCODED_LENGTH 47

// DataUtils.cpp
void charInfoNXToFFLiCharInfo(FFLiCharInfo* dest, const charInfo* src);
void studioToCharInfoNX(charInfo* dest, const charInfoStudio* src);
void studioURLObfuscationDecode(char* data);
void coreDataToCharInfoNX(charInfo* dest, const coreData* src);
FFLResult pickupCharInfoFromData(FFLiCharInfo* pCharInfo, const void* data, u32 dataLength, bool verifyCRC16);
