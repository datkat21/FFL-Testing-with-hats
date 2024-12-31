#pragma once

struct RenderRequest {
    char     data[96];       // just a buffer that accounts for maximum size
    uint16_t dataLength;     // determines the mii data format
    uint8_t  modelFlag;      // FFLModelType + nose flatten @ bit 4
    // completely changes the response type:
    uint8_t  responseFormat; // indicates if response is gltf or tga

    // note that arbitrary resolutions CRASH THE BACKEND
    uint16_t resolution;     // resolution for render buffer
    // texture resolution controls mipmap enable (1 << 30)
    int16_t  texResolution;  // FFLResolution/u32, negative = mipmap
    uint8_t  viewType;       // camera
    uint8_t  resourceType;   // FFLResourceType
    uint8_t  shaderType;
    uint8_t  expression;     // used if expressionFlag is all zeroes
    uint32_t expressionFlag[3]; // casted to FFLAllExpressionFlag
                                // used for multiple expressions
    // expressionFlag will only apply in gltf mode for now
    int16_t  cameraRotate[3];
    int16_t  modelRotate[3];
    uint8_t  backgroundColor[4]; // passed to clearcolor

    uint8_t  aaMethod;       // TODO: TO BE USED SOON? POTENTIALLY?
    uint8_t  drawStageMode;  // opa, xlu, all
    bool     verifyCharInfo; // for FFLiVerifyCharInfoWithReason
    bool     verifyCRC16;
    bool     lightEnable;
    int8_t   clothesColor;   // favorite color, -1 for default
    uint8_t  pantsColor;     // corresponds to PantsColor
    int8_t   bodyType;       // BodyType, -1 = default for shader

    uint8_t  instanceCount;
    uint8_t  instanceRotationMode;
    int16_t  lightDirection[3]; // unset if all negative
};
