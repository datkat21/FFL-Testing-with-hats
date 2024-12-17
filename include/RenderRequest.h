// tga header as a prefix for render responses
// most of this is unused though
// NOTE: completely unused because of alignment weeeeeee
/*
struct TGAHeader {
    uint8_t idLength;        // unused (0)
    uint8_t colorMapType;    // always 0 for no color map
    uint8_t imageType;       // image_type_enum, 2 = uncomp_true_color
    int16_t colorMapOrigin;  // unused
    int16_t colorMapLength;  // unused (0)
    uint8_t colorMapDepth;   // unused
    int16_t originX;         // unused (0)
    int16_t originY;         // unused (0)
    int16_t width;
    int16_t height;
    uint8_t bitsPerPixel;
    uint8_t imageDescriptor; // ???
};

    TGAHeader header;
    // set all fields to 0 initially including unused ones
    rio::MemUtil::set(&header, 0, sizeof(TGAHeader));

    header.imageType = 2; // uncomp_true_color
    header.width = width;
    header.height = height;
    // NOTE: client infers rgba or rgb from bpp (ig type is not set for if it is BGR)
    header.bitsPerPixel = rio::TextureFormatUtil::getPixelByteSize(textureFormat) * 8;
    header.imageDescriptor = 8; // 32 = Flag that sets the image origin to the top left
                                // mii tgas set this to 8 for some reason
*/

enum MiiDataInputType
{
    INPUT_TYPE_FFL_MIIDATACORE,
    INPUT_TYPE_FFL_STOREDATA,
    INPUT_TYPE_RFL_CHARDATA,
    INPUT_TYPE_RFL_STOREDATA,
    INPUT_TYPE_NX_CHARINFO,
    INPUT_TYPE_STUDIO_ENCODED,
    // mii studio site decoded URL format/LocalStorage format
    INPUT_TYPE_STUDIO_RAW,
    INPUT_TYPE_NX_COREDATA,
    INPUT_TYPE_NX_STOREDATA,
    // INPUT_TYPE_RFL_CHARDATA_LE
};

enum ShaderType
{
    SHADER_TYPE_WIIU,
    SHADER_TYPE_SWITCH,
    SHADER_TYPE_MIITOMO,
    /* FUTURE OPTIONS:
     * 3ds
       - potentially downscaling
     * wii...????????
       - it would have to be some TEV to GLSL sheeee
     */
    SHADER_TYPE_WIIU_BLINN, // same as wiiu but with mSpecularMode
    SHADER_TYPE_MAX,
};

enum InstanceRotationMode
{
    INSTANCE_ROTATION_MODE_MODEL,
    INSTANCE_ROTATION_MODE_CAMERA,
    INSTANCE_ROTATION_MODE_EXPRESSION, // neewwww
    INSTANCE_ROATATION_MODE_MAX,
};

enum ViewType
{
    VIEW_TYPE_FACE,      // head with body
    VIEW_TYPE_FACE_ONLY, // head only
    VIEW_TYPE_ALL_BODY,
    VIEW_TYPE_FACE_ONLY_FFLMAKEICON,
    // nn::mii::VariableIconBody::StoreCameraMatrix
    VIEW_TYPE_NNMII_VARIABLEICONBODY_VIEW,
    VIEW_TYPE_ALL_BODY_SUGAR,
};

enum DrawStageMode
{
    DRAW_STAGE_MODE_ALL,
    DRAW_STAGE_MODE_OPA_ONLY,
    DRAW_STAGE_MODE_XLU_ONLY,
    DRAW_STAGE_MODE_MASK_ONLY,
};

enum ResponseFormat
{
    RESPONSE_FORMAT_TGA_DEFAULT,     // with tga header
    RESPONSE_FORMAT_GLTF_MODEL,      // glTF model export
    RESPONSE_FORMAT_TGA_BGRA_FLIP_Y, // RIO_IS_WIN only
};

struct RenderRequest
{
    char data[96];       // just a buffer that accounts for maximum size
    uint16_t dataLength; // determines the mii data format
    uint8_t modelFlag;   // FFLModelType + nose flatten @ bit 4
    // completely changes the response type:
    uint8_t responseFormat; // indicates if response is gltf or tga
    // NOTE: arbitrary resolutions CRASH THE BACKEND
    uint16_t resolution; // resolution for render buffer
    // texture resolution can control whether mipmap is enabled (1 << 30)
    int16_t texResolution; // FFLResolution/u32, negative = mipmap
    uint8_t viewType;      // camera
    uint8_t resourceType;  // FFLResourceType
    uint8_t shaderType;
    uint8_t expression;         // used if expressionFlag is 0
    uint32_t expressionFlag[3]; // for multiple expressions
    // expressionFlag will only apply in gltf mode for now
    int16_t cameraRotate[3];
    int16_t modelRotate[3];
    uint8_t backgroundColor[4]; // passed to clearcolor
    // uint8_t      clothesColor[4]; // fourth color is NOT alpha
    //  but if fourth byte in clothesColor is 0xFF then it is treated as a color instead of an index

    uint8_t aaMethod;      // TODO: TO BE USED SOON? POTENTIALLY?
    uint8_t drawStageMode; // opa, xlu, all
    bool verifyCharInfo;   // for FFLiVerifyCharInfoWithReason
    bool verifyCRC16;
    bool lightEnable;
    int8_t clothesColor; // favorite color, -1 for default
    uint8_t pantsColor;  // corresponds to PantsColor

    uint8_t instanceCount; // TODO
    uint8_t instanceRotationMode;
    // bool          setLightDirection;
    // int16_t       lightDirection[3];

    uint8_t hatType;  // custom!
    uint8_t hatColor; // custom!
};
