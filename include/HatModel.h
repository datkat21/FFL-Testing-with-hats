#pragma once

#include <math/rio_MathTypes.h>

#include <nn/ffl.h>
#include <gfx/mdl/rio_Model.h>

#include <IShader.h>
#include <Model.h>
#include <Types.h>

class HatModel
{
public:
    HatModel(rio::mdl::Model* pHatModel);
    ~HatModel();

    void initialize(Model* pModel, uint8_t hatColor);

    void draw(rio::Matrix34f& model_mtx, rio::BaseMtx34f& view_mtx,
    rio::BaseMtx44f& proj_mtx);

private:
    const rio::mdl::Model* mpHatModel;
    Model*                 mpModel;
    FFLColor               mHatColor; // aka favorite color/hat color

    IShader*               mpShader;
};
