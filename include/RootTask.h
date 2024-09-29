#include <Shader.h>
#include <ShaderSwitch.h>
#include <ShaderMiitomo.h>

#include <gfx/rio_Camera.h>
#include <gfx/mdl/rio_Model.h>
#include <task/rio_Task.h>


#include <nn/ffl.h>
#include <nn/ffl/FFLiMiiData.h>

#define FFLICHARINFO_SIZE sizeof(FFLiCharInfo)

#if RIO_IS_WIN
#include <vector>
#endif

class Model;

enum MiiDataInputType {
    INPUT_TYPE_FFL_MIIDATACORE = 0,
    INPUT_TYPE_FFL_STOREDATA,
    INPUT_TYPE_RFL_CHARDATA,
    INPUT_TYPE_RFL_STOREDATA,
    INPUT_TYPE_NX_CHARINFO,
    INPUT_TYPE_STUDIO_ENCODED,
    // mii studio site decoded URL format/LocalStorage format
    INPUT_TYPE_STUDIO_RAW,
    INPUT_TYPE_NX_COREDATA,
    INPUT_TYPE_NX_STOREDATA,
    //INPUT_TYPE_RFL_CHARDATA_LE
};

enum ShaderType {
    SHADER_TYPE_WIIU,
    SHADER_TYPE_SWITCH,
    SHADER_TYPE_MIITOMO,
    /* FUTURE OPTIONS:
     * 3ds
       - potentially downscaling
     * wii...????????
       - it would have to be some TEV to GLSL sheeee
     */
    SHADER_TYPE_MAX = 3,
};

enum InstanceRotationMode {
    INSTANCE_ROTATION_MODE_MODEL,
    INSTANCE_ROTATION_MODE_CAMERA,
    INSTANCE_ROTATION_MODE_EXPRESSION, // neewwww
    INSTANCE_ROATATION_MODE_MAX = 3,
};

enum ViewType {
    VIEW_TYPE_FACE,  // head with body
    VIEW_TYPE_FACE_ONLY,  // head only
    VIEW_TYPE_ALL_BODY,
    VIEW_TYPE_FACE_ONLY_FFLMAKEICON,
    // nn::mii::VariableIconBody::StoreCameraMatrix
    VIEW_TYPE_NNMII_VARIABLEICONBODY_VIEW,
    VIEW_TYPE_MAX = 5,
};

enum DrawStageMode {
    DRAW_STAGE_MODE_ALL,
    DRAW_STAGE_MODE_OPA_ONLY,
    DRAW_STAGE_MODE_XLU_ONLY,
    DRAW_STAGE_MODE_MASK_ONLY,
    DRAW_STAGE_MODE_MAX = 4,
};

struct RenderRequest {
    char     data[96];       // just a buffer that accounts for maximum size
    uint16_t dataLength;     // determines the mii data format
    uint8_t  modelType;      // head model type, FFLModelType
    bool     exportAsGLTF;   // completely changes the response type
    // NOTE: arbitrary resolutions CRASH THE BACKEND
    uint16_t resolution;     // resolution for render buffer
    // texture resolution can control whether mipmap is enabled (1 << 30)
    int16_t  texResolution;  // FFLResolution/u32, negative = mipmap
    uint8_t  viewType;       // camera
    uint8_t  resourceType;   // FFLResourceType
    uint8_t  shaderType;
    uint8_t  expression;     // used if expressionFlag is 0
    uint32_t expressionFlag; // for multiple expressions
    // expressionFlag will only apply in gltf mode for now
    int16_t  cameraRotate[3];
    int16_t  modelRotate[3];
    uint8_t  backgroundColor[4]; // passed to clearcolor
    //uint8_t      clothesColor[4]; // fourth color is NOT alpha
    // but if fourth byte in clothesColor is 0xFF then it is treated as a color instead of an index

    uint8_t  aaMethod;       // TODO: TO BE USED SOON? POTENTIALLY?
    uint8_t  drawStageMode;  // opa, xlu, all
    bool     verifyCharInfo; // for FFLiVerifyCharInfoWithReason
    bool     verifyCRC16;
    bool     lightEnable;
    int8_t   clothesColor;   // favorite color, -1 for default

    uint8_t  instanceCount;  // TODO
    uint8_t  instanceRotationMode;
    //bool          setLightDirection;
    //int16_t       lightDirection[3];
};
#define RENDERREQUEST_SIZE sizeof(RenderRequest)

#include <mii_ext_MiiPort.h> // used for below function
FFLResult pickupCharInfoFromRenderRequest(FFLiCharInfo* pCharInfo, RenderRequest *buf);

class RootTask : public rio::ITask
{
public:
    RootTask();

private:
    void prepare_() override;
    void calc_() override;
    void exit_() override;

    void handleRenderRequest(char* buf, rio::BaseMtx34f view_mtx);
#ifndef NO_GLTF
    void handleGLTFRequest(RenderRequest* renderRequest);
#endif

    void createModel_();
    //void createModel_(char (*buf)[FFLICHARINFO_SIZE]);
    bool createModel_(RenderRequest* buf, int socket_handle);

    void initializeShaders_() {
        for (int type = 0; type < SHADER_TYPE_MAX; type++) {
            switch (type) {
                case SHADER_TYPE_WIIU:
                    mpShaders[type] = new Shader();
                    break;
                case SHADER_TYPE_SWITCH:
                    mpShaders[type] = new ShaderSwitch();
                    break;
                case SHADER_TYPE_MIITOMO:
                    // miitomo shader needs wii u shader to draw mask
                    mpShaders[type] = new ShaderMiitomo(mpShaders[SHADER_TYPE_WIIU]);
                    break;
            }
            mpShaders[type]->initialize();
        }
    }
    #if RIO_IS_WIN
    void fillStoreDataArray_();
    void setupSocket_();
    #endif

    // NOTE: bc of the amount of arguments, consider putting this in mpModel
    void drawMiiBodyREAL(bool light_enable, FFLiCharInfo* charInfo, rio::Matrix34f& model_mtx, rio::BaseMtx34f& view_mtx, rio::BaseMtx44f& proj_mtx);

private:
    bool                mInitialized;
    bool                mSocketIsListening;
    #if RIO_IS_WIN
    std::vector<std::vector<char>> mStoreDataArray;
    #endif
    FFLResourceDesc     mResourceDesc;
    IShader*            mpShaders[SHADER_TYPE_MAX];
    rio::BaseMtx44f     mProjMtx;
    rio::BaseMtx44f*    mProjMtxIconBody;
    rio::LookAtCamera   mCamera;
    f32                 mCounter;
    s32                 mMiiCounter;
    Model*              mpModel;
    rio::mdl::Model*    mpBodyModels[FFL_GENDER_MAX];
    const char*         mpServerOnly;
    const char*         mpNoSpin;
};
