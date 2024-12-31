#pragma once
#include <Types.h>
#include <nn/ffl/FFLGender.h>

// Mapping of body type strings to names used in the files
const char* cBodyTypeStrings[BODY_TYPE_MAX] = {
    "wiiu",       // BODY_TYPE_WIIU_MIIBODYMIDDLE
    "switch",     // BODY_TYPE_SWITCH_MIIBODYHIGH
    "miitomo",    // BODY_TYPE_MIITOMO
    "fflbodyres"  // BODY_TYPE_FFLBODYRES
};

const char* cBodyGenderStrings[FFL_GENDER_MAX] = {
    "male",  // FFL_GENDER_MALE
    "female" // FFL_GENDER_FEMALE
};

const char* cBodyFileNameFormat = "body/mii_static_body_%s_%s";
                                         // body type_gender
