#include <Shader.h>
#include <ShaderSwitch.h>
#include <ShaderMiitomo.h>

#include <gfx/rio_Camera.h>
#include <gfx/mdl/rio_Model.h>
#include <task/rio_Task.h>

#include <nn/ffl.h>
#include <nn/ffl/FFLiMiiData.h>

#include <RenderRequest.h>

#define FFLICHARINFO_SIZE sizeof(FFLiCharInfo)

#if RIO_IS_WIN
#include <vector>
#endif

// arian told me to put it in here instead of another one so ok i will do that
typedef enum BodyType
{
    BODY_TYPE_WIIU,
    BODY_TYPE_SWITCH,
    BODY_TYPE_MIITOMO,
    BODY_TYPE_MAX,
} BodyType;


class Model;

#define RENDERREQUEST_SIZE sizeof(RenderRequest)

#include <mii_ext_MiiPort.h> // used for below function
FFLResult pickupCharInfoFromRenderRequest(FFLiCharInfo *pCharInfo, RenderRequest *buf);

class RootTask : public rio::ITask
{
public:
    RootTask();

private:
    void prepare_() override;
    void calc_() override;
    void exit_() override;

    void handleRenderRequest(char *buf);
#ifndef NO_GLTF
    void handleGLTFRequest(RenderRequest *renderRequest);
#endif

    void createModel_();
    // void createModel_(char (*buf)[FFLICHARINFO_SIZE]);
    bool createModel_(RenderRequest *buf, int socket_handle);

    void initializeShaders_()
    {
        for (int type = 0; type < SHADER_TYPE_MAX; type++)
        {
            switch (type)
            {
            case SHADER_TYPE_WIIU:
                mpShaders[type] = new Shader();
                break;
            case SHADER_TYPE_WIIU_BLINN:
            {
                Shader *s = new Shader();
                s->setSpecularMode(FFL_SPECULAR_MODE_BLINN);
                mpShaders[type] = s;
                break;
            }
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
    void drawMiiBody(Model *pModel, PantsColor pantsColor, uint8_t bodyType,
                     rio::Matrix34f &model_mtx, rio::BaseMtx34f &view_mtx,
                     rio::BaseMtx44f &proj_mtx, const rio::Vector3f scaleFactors);
    void drawMiiHat(Model *pModel, uint8_t &hatType,
                    rio::Matrix34f &model_mtx, rio::BaseMtx34f &view_mtx,
                    rio::BaseMtx44f &proj_mtx, const rio::Vector3f scaleFactors);

    void setViewTypeParams(ViewType viewType, rio::LookAtCamera *pCamera, rio::BaseMtx44f *projMtx, float *aspectHeightFactor, bool *isCameraPosAbsolute, bool *willDrawBody, FFLiCharInfo *pCharInfo);

private:
    bool mInitialized;
    bool mSocketIsListening;
#if RIO_IS_WIN
    std::vector<std::vector<char>> mStoreDataArray;
#endif
    FFLResourceDesc mResourceDesc;
    IShader *mpShaders[SHADER_TYPE_MAX];
    rio::BaseMtx44f mProjMtx;
    rio::BaseMtx44f *mProjMtxIconBody;
    rio::LookAtCamera mCamera;
    f32 mCounter;
    s32 mMiiCounter;
    Model *mpModel;
    rio::mdl::Model *mpBodyModels[6];
    rio::mdl::Model *mpHatModels[10];
    const char *mpServerOnly;
    const char *mpNoSpin;
};
