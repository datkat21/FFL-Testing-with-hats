#include <HatModel.h>
#include <Types.h>

HatModel::HatModel(rio::mdl::Model* pHatModel)
    : mpModel(nullptr)
    , mHatColor{ 0.0f, 0.0f, 0.0f, 1.0f }
    , mpShader(nullptr)
{
    // init local hat model
    mpHatModel = pHatModel;
}

HatModel::~HatModel() { }

void HatModel::initialize(Model* pModel, uint8_t hatColor)
{
    mpModel = pModel;

    // Position hat model accordingly!

    FFLiCharInfo* pCharInfo = &reinterpret_cast<FFLiCharModel*>(mpModel->getCharModel())->charInfo;

    uint8_t tmpHatColor = hatColor;

    // hacky to prevent above favorite color?
    if (hatColor != 0 && hatColor < 12) 
    {
        // subtract one for miic compatibility...
        tmpHatColor = hatColor - 1;
    } 
    else 
    {
        tmpHatColor = pCharInfo->favoriteColor;
    }

    mHatColor = FFLGetFavoriteColor(tmpHatColor);
}

// draws mii hat based on passed hat model 
// shader sets hat color accordingly
void HatModel::draw(rio::Matrix34f& model_mtx, rio::BaseMtx34f& view_mtx, rio::BaseMtx44f& proj_mtx)
{
    const bool lightEnable = mpModel->getLightEnable();
    FFLiCharInfo* pCharInfo = mpModel->getCharInfo();

    const rio::mdl::Mesh* meshes = mpHatModel->meshes(); // Hat mesh.

    // Render each mesh in order
    for (u32 i = 0; i < mpHatModel->numMeshes(); i++)
    {
        const rio::mdl::Mesh& mesh = meshes[i];

        // Bind shader and set hat material.
        IShader* pShader = mpModel->getShader();
        pShader->bind(lightEnable, pCharInfo);

        const FFLColor modulateColor = mHatColor;
        const FFLModulateParam modulateParam = {
            FFL_MODULATE_MODE_CONSTANT, // no texture for now
            CUSTOM_MATERIAL_PARAM_HAT, // decides which material is bound
            &modulateColor, // constant color (R)
            nullptr, // no color G
            nullptr, // no color B
            nullptr  // no texture
        };

        pShader->setModulate(modulateParam);

        // make new matrix for body
        rio::Matrix34f modelMtxHat = rio::Matrix34f::ident;//model_mtx;

        // apply original model matrix (rotation)
        modelMtxHat.setMul(model_mtx, modelMtxHat);

        pShader->setViewUniform(modelMtxHat, view_mtx, proj_mtx);

        rio::RenderState render_state;
        render_state.setCullingMode(rio::Graphics::CULLING_MODE_BACK);
        render_state.applyCullingAndPolygonModeAndPolygonOffset();
        mesh.draw();
    }
}