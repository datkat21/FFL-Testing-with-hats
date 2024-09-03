#pragma once

#include <IShader.h>

#include <math/rio_Vector.h>

#include <gpu/rio_TextureSampler.h>

#if RIO_IS_CAFE
#include <gx2/shaders.h>
#endif // RIO_IS_CAFE

class ShaderMiitomo : public IShader
{
public:
    ShaderMiitomo(IShader* mpMaskShader);
    ~ShaderMiitomo();

    void initialize() override;

    void bind(bool light_enable, FFLiCharInfo* charInfo) override;

    void bindBodyShader(bool light_enable, FFLiCharInfo* pCharInfo) override;

    void setViewUniform(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const;
    void setViewUniformBody(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const override {
        setViewUniform(model_mtx, view_mtx, proj_mtx);
    };

    void applyAlphaTestEnable() const override
    {
        applyAlphaTest(true, rio::Graphics::COMPARE_FUNC_GREATER, 0.0f);
    }

    void applyAlphaTestDisable() const override
    {
        applyAlphaTest(false, rio::Graphics::COMPARE_FUNC_ALWAYS, 0.0f);
    }

    void applyAlphaTest(bool enable, rio::Graphics::CompareFunc func, f32 ref) const override;

    static void setCulling(FFLCullMode mode);

private:
    static void applyAlphaTestCallback_(void* p_obj, bool enable, rio::Graphics::CompareFunc func, f32 ref);
    void setShaderCallback_();

    void bindTexture_(const FFLModulateParam& modulateParam);
    void setConstColor_(u32 ps_loc, FFLColor color);
    void setModulateMode_(FFLModulateMode mode);
    void setModulate_(const FFLModulateParam& modulateParam);

    void setMaterial_(const FFLModulateType modulateType);

    void draw_(const FFLDrawParam& draw_param);
    static void drawCallback_(void* p_obj, const FFLDrawParam& draw_param);

    void setMatrix_(const rio::BaseMtx44f& matrix);
    static void setMatrixCallback_(void* p_obj, const rio::BaseMtx44f& matrix);

private:
    enum VertexUniform
    {
        VERTEX_UNIFORM_MVP = 0,  // Inverse transpose of MV
        VERTEX_UNIFORM_MODEL,
        VERTEX_UNIFORM_NORMAL,
        VERTEX_UNIFORM_ALPHA,
        VERTEX_UNIFORM_BONE_COUNT,
        VERTEX_UNIFORM_HS_LIGHT_GROUND_COLOR,
        VERTEX_UNIFORM_HS_LIGHT_SKY_COLOR,
        VERTEX_UNIFORM_LIGHT_DIR_AND_TYPE0,
        VERTEX_UNIFORM_LIGHT_DIR_AND_TYPE1,
        VERTEX_UNIFORM_EYE_PT,
        VERTEX_UNIFORM_DIR_LIGHT_COUNT,
        VERTEX_UNIFORM_DIR_LIGHT_COLOR0,
        VERTEX_UNIFORM_DIR_LIGHT_COLOR1,
        VERTEX_UNIFORM_MAX
    };

    enum PixelUniform
    {
        PIXEL_UNIFORM_MODE = 0,
        PIXEL_UNIFORM_ALPHA_TEST,
        PIXEL_UNIFORM_CONST1,
        PIXEL_UNIFORM_CONST2,
        PIXEL_UNIFORM_CONST3,
        PIXEL_UNIFORM_LIGHT_COLOR,
        PIXEL_UNIFORM_LIGHT_ENABLE,
        PIXEL_UNIFORM_MAX
    };

    rio::Shader             mShader;
    IShader*                mpMaskShader;
    s32                     mVertexUniformLocation[VERTEX_UNIFORM_MAX];
    s32                     mPixelUniformLocation[PIXEL_UNIFORM_MAX];
    s32                     mSamplerLocation;
    s32                     mLUTSpecSamplerLocation;
    s32                     mLUTFresSamplerLocation;
    s32                     mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_MAX];
#if RIO_IS_CAFE
    GX2AttribStream         mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_MAX];
    GX2FetchShader          mFetchShader;
#elif RIO_IS_WIN
    u32                     mVBOHandle[FFL_ATTRIBUTE_BUFFER_TYPE_MAX];
    u32                     mVAOHandle;
#endif
    FFLShaderCallback       mCallback;
    rio::TextureSampler2D   mSampler;
    FFLiCharInfo*           mpCharInfo;
    bool                    mLightEnable;
    rio::TextureSampler2D   mLUTSpecSampler;
    rio::TextureSampler2D   mLUTFresSampler;
};
