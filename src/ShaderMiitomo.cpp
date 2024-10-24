#include "math/rio_MathTypes.h"
#include <ShaderMiitomo.h>

#include <fstream>
#include <gpu/rio_RenderState.h>
#include <math/rio_Matrix.h>
#include <misc/rio_MemUtil.h>

#include <filedevice/rio_FileDeviceMgr.h>

#include <gpu/win/rio_Texture2DUtilWin.h>
#include <gpu/rio_TextureSampler.h>

#if RIO_IS_CAFE
#include <gx2/registers.h>
#include <gx2/utils.h>
#endif // RIO_IS_CAFE

namespace {

#if RIO_IS_CAFE

#define GX2_ATTRIB_FORMAT_SNORM_10_10_10_2 (GX2AttribFormat(GX2_ATTRIB_FLAG_SIGNED | GX2_ATTRIB_TYPE_10_10_10_2))
#define GX2_SHADER_ALIGNMENT GX2_SHADER_PROGRAM_ALIGNMENT

#define GX2_COMP_SEL_X001 GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_0, GX2_SQ_SEL_0, GX2_SQ_SEL_1)
#define GX2_COMP_SEL_XY01 GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_0, GX2_SQ_SEL_1)
#define GX2_COMP_SEL_XYZ1 GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_Z, GX2_SQ_SEL_1)
#define GX2_COMP_SEL_XYZW GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_Z, GX2_SQ_SEL_W)

inline void GX2InitAttribStream(
    GX2AttribStream* p_attribute,
    u32 location,
    u32 buffer,
    u32 offset,
    GX2AttribFormat format
)
{
    static constexpr u32 sFormatMask[] = {
        GX2_COMP_SEL_X001, GX2_COMP_SEL_XY01,
        GX2_COMP_SEL_X001, GX2_COMP_SEL_X001,
        GX2_COMP_SEL_XY01, GX2_COMP_SEL_X001,
        GX2_COMP_SEL_X001, GX2_COMP_SEL_XY01,
        GX2_COMP_SEL_XY01, GX2_COMP_SEL_XYZ1,
        GX2_COMP_SEL_XYZW, GX2_COMP_SEL_XYZW,
        GX2_COMP_SEL_XY01, GX2_COMP_SEL_XY01,
        GX2_COMP_SEL_XYZW, GX2_COMP_SEL_XYZW,
        GX2_COMP_SEL_XYZ1, GX2_COMP_SEL_XYZ1,
        GX2_COMP_SEL_XYZW, GX2_COMP_SEL_XYZW
    };

    p_attribute->buffer     = buffer;
    p_attribute->offset     = offset;
    p_attribute->location   = location;
    p_attribute->format     = format;
    p_attribute->mask       = sFormatMask[format & 0xff];
    p_attribute->type       = GX2_ATTRIB_INDEX_PER_VERTEX;
    p_attribute->aluDivisor = 0;
    p_attribute->endianSwap = GX2_ENDIAN_SWAP_DEFAULT;
}

#endif // RIO_IS_CAFE


void safeNormalizeVec3(rio::BaseVec3f* vec) {
    float magnitude = std::sqrt(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
    if (magnitude != 0.0f) {
        vec->x /= magnitude;
        vec->y /= magnitude;
        vec->z /= magnitude;
    }

    vec->x = std::fmax(std::fmin(vec->x, 1.0f), -1.0f);
    vec->y = std::fmax(std::fmin(vec->y, 1.0f), -1.0f);
    vec->z = std::fmax(std::fmin(vec->z, 1.0f), -1.0f);

    if (vec->x == 1.0f || vec->x == -1.0f) {
        vec->y = 0.0f;
        vec->z = 0.0f;
    } else if (vec->y == 1.0f || vec->y == -1.0f) {
        vec->x = 0.0f;
        vec->z = 0.0f;
    } else if (vec->z == 1.0f || vec->z == -1.0f) {
        vec->x = 0.0f;
        vec->y = 0.0f;
    }
}

void gramSchmidtOrthonormalizeMtx34(rio::BaseMtx34f* mat)
{
    rio::BaseVec3f c0, c0Normalized, c1, c1Normalized, c1New, c2New;

    // Extract and normalize the first column
    c0.x = mat->m[0][0];
    c0.y = mat->m[1][0];
    c0.z = mat->m[2][0];
    c0Normalized = c0;
    safeNormalizeVec3(&c0Normalized);

    // Extract and normalize the second column
    c1.x = mat->m[0][1];
    c1.y = mat->m[1][1];
    c1.z = mat->m[2][1];
    c1Normalized = c1;
    safeNormalizeVec3(&c1Normalized);

    // Compute the third column as the cross product of the first two normalized columns
    c2New.x = c0Normalized.y * c1Normalized.z - c0Normalized.z * c1Normalized.y;
    c2New.y = c0Normalized.z * c1Normalized.x - c0Normalized.x * c1Normalized.z;
    c2New.z = c0Normalized.x * c1Normalized.y - c0Normalized.y * c1Normalized.x;

    // Compute the new second column as the cross product of the third column and the first normalized column
    c1New.x = c2New.y * c0Normalized.z - c2New.z * c0Normalized.y;
    c1New.y = c2New.z * c0Normalized.x - c2New.x * c0Normalized.z;
    c1New.z = c2New.x * c0Normalized.y - c2New.y * c0Normalized.x;

    // Update the matrix with the new orthonormal columns
    mat->m[0][0] = c0Normalized.x;
    mat->m[1][0] = c0Normalized.y;
    mat->m[2][0] = c0Normalized.z;

    mat->m[0][1] = c1New.x;
    mat->m[1][1] = c1New.y;
    mat->m[2][1] = c1New.z;

    mat->m[0][2] = c2New.x;
    mat->m[1][2] = c2New.y;
    mat->m[2][2] = c2New.z;
}

const float cAlpha = 1.00f;

const rio::BaseVec4f cLightDirAndType0 = { -0.20f, 0.50f, 0.80f, -1.00f };
const rio::BaseVec4f cLightDirAndType1 = { 0.00f, -0.19612f, 0.98058f, -1.00f };
const int cDirLightCount = 2;


const rio::BaseVec3f cDirLightColor0 = { 0.35137f, 0.32392f, 0.32392f };
const rio::BaseVec3f cDirLightColor1 = { 0.10039f, 0.09255f, 0.09255f };


const rio::BaseVec3f cLightColor = { 0.35137f, 0.32392f, 0.32392f };

const rio::BaseVec3f cHSLightGroundColor = { 0.87843f, 0.72157f, 0.5898f };
const rio::BaseVec3f cHSLightSkyColor = { 0.87843f, 0.83451f, 0.80314f };


}

ShaderMiitomo::ShaderMiitomo(IShader* mpMaskShader)
    : mpMaskShader(mpMaskShader)
#if RIO_IS_CAFE
    , mAttribute()
    , mFetchShader()
#elif RIO_IS_WIN
    , mVBOHandle()
    , mVAOHandle()
#endif

{
    rio::MemUtil::set(mVertexUniformLocation, u8(-1), sizeof(mVertexUniformLocation));
    rio::MemUtil::set(mPixelUniformLocation, u8(-1), sizeof(mPixelUniformLocation));
    mSamplerLocation = -1;
    rio::MemUtil::set(mAttributeLocation, u8(-1), sizeof(mAttributeLocation));
    mLUTSpecSamplerLocation = -1;
    mLUTFresSamplerLocation = -1;
}

// names not intentional
const int cMaterialParamSize = 10;
rio::Texture2D* sLUTSpecTextures[cMaterialParamSize];
rio::Texture2D* sLUTFresTextures[cMaterialParamSize];
// 9 for each draw type

ShaderMiitomo::~ShaderMiitomo()
{
#if RIO_IS_CAFE
    if (mFetchShader.program != nullptr)
    {
        rio::MemUtil::free(mFetchShader.program);
        mFetchShader.program = nullptr;
    }
#elif RIO_IS_WIN
    if (mVAOHandle != GL_NONE)
    {
        RIO_GL_CALL(glDeleteVertexArrays(1, &mVAOHandle));
        RIO_GL_CALL(glDeleteBuffers(FFL_ATTRIBUTE_BUFFER_TYPE_MAX, mVBOHandle));
        rio::MemUtil::set(mVBOHandle, 0, sizeof(mVBOHandle));
        mVAOHandle = GL_NONE;
    }
#endif
    for (u32 i = 0; i < cMaterialParamSize; i++)
        delete sLUTSpecTextures[i];
    for (u32 i = 0; i < cMaterialParamSize; i++)
        delete sLUTFresTextures[i];
}

void loadRawR8Image(const char* filePath, int width, int height, GLuint texture) {
    // Use rio::FileDeviceMgr to load the file
    rio::FileDevice::LoadArg arg;
    arg.path = std::string("miitomo_shader_lut/") + filePath;
    arg.alignment = 0x2000;  // Adjust alignment if necessary

    // not the native file device so it will be in fs/content/
    u8* data = rio::FileDeviceMgr::instance()->tryLoad(arg);

    if (data == nullptr) {
        RIO_LOG("NativeFileDevice failed to load when trying to load LUT for ShaderMiitomo: %s\n", filePath);
        return;
    }

    u32 imageSize = width * height * 1; // 1 channel (R)

    if (arg.read_size != imageSize) {
        RIO_LOG("Failed to load the correct size: %s\n", filePath);
        return;
    }

    /*GLuint texture;
    glGenTextures(1, &texture);
    */
    RIO_GL_CALL(glBindTexture(GL_TEXTURE_2D, texture));

    //RIO_GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data));
    RIO_GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, data));
    delete[] data;

    RIO_GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));

    return;//texture;
}

void ShaderMiitomo::initialize()
{
    mShader.load("LUT", rio::Shader::MODE_UNIFORM_REGISTER);

    mVertexUniformLocation[VERTEX_UNIFORM_MVP] = mShader.getVertexUniformLocation("uMVPMatrix");
    mVertexUniformLocation[VERTEX_UNIFORM_MODEL] = mShader.getVertexUniformLocation("uModelMatrix");
    mVertexUniformLocation[VERTEX_UNIFORM_NORMAL] = mShader.getVertexUniformLocation("uNormalMatrix");
    mVertexUniformLocation[VERTEX_UNIFORM_ALPHA] = mShader.getVertexUniformLocation("uAlpha");
    mVertexUniformLocation[VERTEX_UNIFORM_BONE_COUNT] = mShader.getVertexUniformLocation("uBoneCount");
    mVertexUniformLocation[VERTEX_UNIFORM_HS_LIGHT_GROUND_COLOR] = mShader.getVertexUniformLocation("uHSLightGroundColor");
    mVertexUniformLocation[VERTEX_UNIFORM_HS_LIGHT_SKY_COLOR] = mShader.getVertexUniformLocation("uHSLightSkyColor");
    mVertexUniformLocation[VERTEX_UNIFORM_LIGHT_DIR_AND_TYPE0] = mShader.getVertexUniformLocation("uDirLightDirAndType0");
    mVertexUniformLocation[VERTEX_UNIFORM_LIGHT_DIR_AND_TYPE1] = mShader.getVertexUniformLocation("uDirLightDirAndType1");
    mVertexUniformLocation[VERTEX_UNIFORM_EYE_PT] = mShader.getVertexUniformLocation("uEyePt");
    mVertexUniformLocation[VERTEX_UNIFORM_DIR_LIGHT_COUNT] = mShader.getVertexUniformLocation("uDirLightCount");
    mVertexUniformLocation[VERTEX_UNIFORM_DIR_LIGHT_COLOR0] = mShader.getVertexUniformLocation("uDirLightColor0");
    mVertexUniformLocation[VERTEX_UNIFORM_DIR_LIGHT_COLOR1] = mShader.getVertexUniformLocation("uDirLightColor1");

    // this is CUSTOM
    mPixelUniformLocation[PIXEL_UNIFORM_MODE] = mShader.getFragmentUniformLocation("uMode");
    // this too
    mPixelUniformLocation[PIXEL_UNIFORM_ALPHA_TEST] = mShader.getFragmentUniformLocation("uAlphaTest");
    // finally this
    mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_ENABLE] = mShader.getFragmentUniformLocation("uLightEnable");

    mPixelUniformLocation[PIXEL_UNIFORM_CONST1] = mShader.getFragmentUniformLocation("uColor0");
    mPixelUniformLocation[PIXEL_UNIFORM_CONST2] = mShader.getFragmentUniformLocation("uColor1");
    mPixelUniformLocation[PIXEL_UNIFORM_CONST3] = mShader.getFragmentUniformLocation("uColor2");
    mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_COLOR] = mShader.getFragmentUniformLocation("uLightColor");

    mSamplerLocation = mShader.getFragmentSamplerLocation("uAlbedoTexture");

    mLUTSpecSamplerLocation =  mShader.getFragmentSamplerLocation("uLUTSpecTexture");
    mLUTFresSamplerLocation =  mShader.getFragmentSamplerLocation("uLUTFresTexture");

    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_POSITION]  = mShader.getVertexAttribLocation("aPosition");
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL]    = mShader.getVertexAttribLocation("aNormal");
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD]  = mShader.getVertexAttribLocation("aTexcoord0");
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_COLOR]     = mShader.getVertexAttribLocation("aColor");
    mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT]   = mShader.getVertexAttribLocation("aTangent");

    // swizzle RRR1
    const u32 compMap = 0x00000005;
    char lutFresFileName[] = "miitomo_lut_fres_r8_0";
    for (u32 i = 0; i < cMaterialParamSize; i++) {
        sLUTFresTextures[i] = new rio::Texture2D(rio::TEXTURE_FORMAT_R8_UNORM, 512, 1, 1);
        loadRawR8Image(lutFresFileName, 512, 1, sLUTFresTextures[i]->getNativeTextureHandle());
        rio::Texture2DUtil::setSwizzle(sLUTFresTextures[i]->getNativeTextureHandle(), compMap);
        lutFresFileName[sizeof(lutFresFileName)-2]++; // iterate last character
    }

    char lutSpecFileName[] = "miitomo_lut_spec_r8_0";
    for (u32 i = 0; i < cMaterialParamSize; i++) {
        sLUTSpecTextures[i] = new rio::Texture2D(rio::TEXTURE_FORMAT_R8_UNORM, 512, 1, 1);
        loadRawR8Image(lutSpecFileName, 512, 1, sLUTSpecTextures[i]->getNativeTextureHandle());
        rio::Texture2DUtil::setSwizzle(sLUTSpecTextures[i]->getNativeTextureHandle(), compMap);
        lutSpecFileName[sizeof(lutSpecFileName)-2]++; // iterate last character
    }

#if RIO_IS_CAFE
    GX2InitAttribStream(
        &(mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_POSITION]),
        mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_POSITION],
        FFL_ATTRIBUTE_BUFFER_TYPE_POSITION,
        0,
        GX2_ATTRIB_FORMAT_FLOAT_32_32_32
    );
    GX2InitAttribStream(
        &(mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD]),
        mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD],
        FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD,
        0,
        GX2_ATTRIB_FORMAT_FLOAT_32_32
    );
    GX2InitAttribStream(
        &(mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL]),
        mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL],
        FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL,
        0,
        GX2_ATTRIB_FORMAT_SNORM_10_10_10_2
    );
    GX2InitAttribStream(
        &(mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT]),
        mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT],
        FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT,
        0,
        GX2_ATTRIB_FORMAT_SNORM_8_8_8_8
    );
    GX2InitAttribStream(
        &(mAttribute[FFL_ATTRIBUTE_BUFFER_TYPE_COLOR]),
        mAttributeLocation[FFL_ATTRIBUTE_BUFFER_TYPE_COLOR],
        FFL_ATTRIBUTE_BUFFER_TYPE_COLOR,
        0,
        GX2_ATTRIB_FORMAT_UNORM_8_8_8_8
    );

    u32 size = GX2CalcFetchShaderSizeEx(FFL_ATTRIBUTE_BUFFER_TYPE_MAX, GX2_FETCH_SHADER_TESSELLATION_NONE, GX2_TESSELLATION_MODE_DISCRETE);
    void* buffer = rio::MemUtil::alloc(size, GX2_SHADER_ALIGNMENT);
    GX2InitFetchShaderEx(&mFetchShader, (u8*)buffer, FFL_ATTRIBUTE_BUFFER_TYPE_MAX, mAttribute, GX2_FETCH_SHADER_TESSELLATION_NONE, GX2_TESSELLATION_MODE_DISCRETE);
#elif RIO_IS_WIN
    RIO_ASSERT(mVAOHandle == GL_NONE);
    RIO_GL_CALL(glGenBuffers(FFL_ATTRIBUTE_BUFFER_TYPE_MAX, mVBOHandle));
    RIO_GL_CALL(glGenVertexArrays(1, &mVAOHandle));
    RIO_ASSERT(mVAOHandle != GL_NONE);
#endif

    mSampler.setWrap(rio::TEX_WRAP_MODE_MIRROR, rio::TEX_WRAP_MODE_MIRROR, rio::TEX_WRAP_MODE_MIRROR);
    mSampler.setLOD(-1000.0f, 1000.0f, 0.0f);
    mSampler.setFilter(rio::TEX_XY_FILTER_MODE_LINEAR, rio::TEX_XY_FILTER_MODE_LINEAR, rio::TEX_MIP_FILTER_MODE_LINEAR, rio::TEX_ANISO_1_TO_1);

    mCallback.pObj = this;
    mCallback.pApplyAlphaTestFunc = &ShaderMiitomo::applyAlphaTestCallback_;
    mCallback.pDrawFunc = &ShaderMiitomo::drawCallback_;
    mCallback.pSetMatrixFunc = &ShaderMiitomo::setMatrixCallback_;
    setShaderCallback_();
}

void ShaderMiitomo::setShaderCallback_()
{
    FFLSetShaderCallback(&mCallback);
}

void ShaderMiitomo::bind(bool light_enable, FFLiCharInfo* pCharInfo)
{
    // ASSUME this is drawing mask/faceline texture in which case the LUT shader cannot be used for that
    mLightEnable = light_enable;
    if (!light_enable) {
        return mpMaskShader->bind(light_enable, pCharInfo);
    }

    mpCharInfo = pCharInfo;
    mShader.bind();
    setShaderCallback_();

    #if RIO_IS_CAFE
    GX2SetFetchShader(&mFetchShader);
#elif RIO_IS_WIN
    RIO_GL_CALL(glBindVertexArray(mVAOHandle));
    for (u32 i = 0; i < FFL_ATTRIBUTE_BUFFER_TYPE_MAX; i++)
        RIO_GL_CALL(glDisableVertexAttribArray(i));
#endif

    mShader.setUniform(cAlpha, mVertexUniformLocation[VERTEX_UNIFORM_ALPHA], u32(-1));
    mShader.setUniform(0, mVertexUniformLocation[VERTEX_UNIFORM_BONE_COUNT], u32(-1));

    mShader.setUniform(light_enable, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_ENABLE]);

}

void ShaderMiitomo::setViewUniform(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const
{
    if (!mLightEnable) return mpMaskShader->setViewUniform(model_mtx, view_mtx, proj_mtx);
    mShader.setUniform(-view_mtx.m[0][3], -view_mtx.m[1][3], -view_mtx.m[2][3], mVertexUniformLocation[VERTEX_UNIFORM_EYE_PT], u32(-1));
    //mShader.setUniform(0.0f, 3.45f, 60.0f, mVertexUniformLocation[VERTEX_UNIFORM_EYE_PT], u32(-1));
    rio::Matrix34f mv;
    mv.setMul(static_cast<const rio::Matrix34f&>(view_mtx), static_cast<const rio::Matrix34f&>(model_mtx));

    rio::Matrix44f mvp;
    mvp.setMul(static_cast<const rio::Matrix44f&>(proj_mtx), mv);
    /*rio::BaseMtx44f mvp {
        11.57407, 0, 0, 0,
        0, 11.57407, 0, -39.93055,
        0, 0, -6, 10,
        0, 0, -1, 60
    };*/
    /* previous mvp
{11.57407, 0.00, 0.00, 0.00}
{0.00, 11.57407, 0.00, 0.00}
{0.00, 0.00, -2.42857, -1714.28564}
{0.00, 0.00, -1.00, 0.00}    float4x4 (column_major)
    */
    mShader.setUniform(mvp, mVertexUniformLocation[VERTEX_UNIFORM_MVP], u32(-1));

    rio::Matrix44f model44;
    model44.fromMatrix34(model_mtx);
    mShader.setUniform(model44, mVertexUniformLocation[VERTEX_UNIFORM_MODEL], u32(-1));

    rio::Matrix34f it34 = mv;
    it34.setInverseTranspose(mv);
    gramSchmidtOrthonormalizeMtx34(&it34);
    rio::BaseMtx33f it {
        it34.m[0][0], it34.m[1][0], it34.m[2][0],
        it34.m[0][1], it34.m[1][1], it34.m[2][1],
        it34.m[0][2], it34.m[1][2], it34.m[2][2]
    };
    mShader.setUniformColumnMajor(it, mVertexUniformLocation[VERTEX_UNIFORM_NORMAL], u32(-1));
}

void ShaderMiitomo::applyAlphaTest(bool enable, rio::Graphics::CompareFunc func, f32 ref) const
{
#if RIO_IS_CAFE
    GX2SetAlphaTest(enable, GX2CompareFunction(func), ref);
#endif
}

void ShaderMiitomo::setCulling(FFLCullMode mode)
{
    if (mode > FFL_CULL_MODE_FRONT)
        return;

    rio::RenderState render_state;

    switch (mode)
    {
    case FFL_CULL_MODE_NONE:
        render_state.setCullingMode(rio::Graphics::CULLING_MODE_NONE);
        break;
    case FFL_CULL_MODE_BACK:
        render_state.setCullingMode(rio::Graphics::CULLING_MODE_BACK);
        break;
    case FFL_CULL_MODE_FRONT:
        render_state.setCullingMode(rio::Graphics::CULLING_MODE_FRONT);
        break;
    default:
        break;
    }

    render_state.applyCullingAndPolygonModeAndPolygonOffset();
}

void ShaderMiitomo::applyAlphaTestCallback_(void* p_obj, bool enable, rio::Graphics::CompareFunc func, f32 ref)
{
    static_cast<ShaderMiitomo*>(p_obj)->applyAlphaTest(enable, func, ref);
}

void ShaderMiitomo::bindTexture_(const FFLModulateParam& modulateParam)
{
    if (modulateParam.pTexture2D != nullptr)
    {
        mSampler.linkTexture2D(modulateParam.pTexture2D);
        mSampler.tryBindFS(mSamplerLocation, 0);
/*
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, modulateParam.pTexture2D->getNativeTextureHandle());
        glUniform1i(glGetUniformLocation(mShader.getShaderProgram(), "uAlbedoTexture"), 0);
*/
    }

    if (modulateParam.type == FFL_MODULATE_TYPE_SHAPE_NOSELINE
        || modulateParam.type > cMaterialParamSize
    )
        return;

    mLUTSpecSampler.linkTexture2D(sLUTSpecTextures[modulateParam.type]);
    mLUTSpecSampler.tryBindFS(mLUTSpecSamplerLocation, 4);

    mLUTFresSampler.linkTexture2D(sLUTFresTextures[modulateParam.type]);
    mLUTFresSampler.tryBindFS(mLUTFresSamplerLocation, 5);
/*
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sLUTFresTextures[modulateParam.type]->getNativeTextureHandle());
    glUniform1i(glGetUniformLocation(mShader.getShaderProgram(), "uLUTFresTexture"), 5);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sLUTSpecTextures[modulateParam.type]->getNativeTextureHandle());
    glUniform1i(glGetUniformLocation(mShader.getShaderProgram(), "uLUTSpecTexture"), 4);
*/
}

FFLColor multiplyColorIfNeeded(FFLModulateType modulateType, FFLColor color)
{
    // faceline/skin color should be excluded from this
    if (
        // forehead, nose etc. use faceline color
        // faceline usually uses texture but if there is no
        // wrinkles or blush then faceline will go through this
        // though that is only because of an optimization

        modulateType == FFL_MODULATE_TYPE_SHAPE_FACELINE
        || modulateType == FFL_MODULATE_TYPE_SHAPE_FOREHEAD
        || modulateType == FFL_MODULATE_TYPE_SHAPE_NOSE
        || modulateType == FFL_MODULATE_TYPE_SHAPE_GLASS
        || modulateType == 9 // body, favorite color
    )
        return color;
    // FUN_0056ba10 in libcocos2dcpp.so
    return FFLColor {
        color.r * 0.9019608f,
        color.g * 0.9019608f,
        color.b * 0.9019608f,
        color.a
    };
}

void ShaderMiitomo::setConstColor_(u32 ps_loc, FFLColor color)
{
    mShader.setUniform(color.r, color.g, color.b, color.a, u32(-1), ps_loc);
}

void ShaderMiitomo::setModulateMode_(FFLModulateMode mode)
{
    mShader.setUniform(s32(mode), u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_MODE]);
}

void ShaderMiitomo::setModulate_(const FFLModulateParam& modulateParam)
{
    setModulateMode_(modulateParam.mode);

    // if you want to change colors based on modulateParam.type
    // FFL_MODULATE_TYPE_SHAPE_HAIR
    // hair color: pColorR/const1
    // FFL_MODULATE_TYPE_EYEBROW
    // eyebrow color: pColorB/const2
    // NOTE, also need to color: FFL_MODULATE_TYPE_SHAPE_BEARD, FFL_MODULATE_TYPE_MUSTACHE, FFL_MODULATE_TYPE_FACE_BEARD

    switch (modulateParam.mode)
    {
    case FFL_MODULATE_MODE_0:
    case FFL_MODULATE_MODE_3:
    case FFL_MODULATE_MODE_4:
    case FFL_MODULATE_MODE_5:
        setConstColor_(mPixelUniformLocation[PIXEL_UNIFORM_CONST1], multiplyColorIfNeeded(modulateParam.type, *modulateParam.pColorR));
        break;
    case FFL_MODULATE_MODE_2:
        setConstColor_(mPixelUniformLocation[PIXEL_UNIFORM_CONST1],
                       multiplyColorIfNeeded(modulateParam.type, *modulateParam.pColorR));
        setConstColor_(mPixelUniformLocation[PIXEL_UNIFORM_CONST2],
                       multiplyColorIfNeeded(modulateParam.type, *modulateParam.pColorG));
        setConstColor_(mPixelUniformLocation[PIXEL_UNIFORM_CONST3],
                       multiplyColorIfNeeded(modulateParam.type, *modulateParam.pColorB));
        break;
    default:
        break;
    }

    bindTexture_(modulateParam);
}

void ShaderMiitomo::setMaterial_(const FFLModulateType modulateType)
{
    if (modulateType == FFL_MODULATE_TYPE_SHAPE_NOSELINE) {
        //mShader.setUniform(0.00f, 0.00f, 0.00f, mVertexUniformLocation[VERTEX_UNIFORM_EYE_PT], u32(-1));
        mShader.setUniform(false, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_ENABLE]);
        return;
    }

    // assuming we are never drawing the mask here
    mShader.setUniform(true, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_ENABLE]);

    mShader.setUniform(cHSLightGroundColor, mVertexUniformLocation[VERTEX_UNIFORM_HS_LIGHT_GROUND_COLOR], u32(-1));
    mShader.setUniform(cHSLightSkyColor, mVertexUniformLocation[VERTEX_UNIFORM_HS_LIGHT_SKY_COLOR], u32(-1));


    mShader.setUniform(cDirLightColor0, mVertexUniformLocation[VERTEX_UNIFORM_DIR_LIGHT_COLOR0], u32(-1));
    mShader.setUniform(cDirLightColor1, mVertexUniformLocation[VERTEX_UNIFORM_DIR_LIGHT_COLOR1], u32(-1));

    mShader.setUniform(cLightDirAndType0, mVertexUniformLocation[VERTEX_UNIFORM_LIGHT_DIR_AND_TYPE0], u32(-1));
    mShader.setUniform(cLightDirAndType1, mVertexUniformLocation[VERTEX_UNIFORM_LIGHT_DIR_AND_TYPE1], u32(-1));
    mShader.setUniform(cDirLightCount, mVertexUniformLocation[VERTEX_UNIFORM_DIR_LIGHT_COUNT], u32(-1));

    mShader.setUniform(cLightColor, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_LIGHT_COLOR]);

    switch (modulateType) {
        case FFL_MODULATE_TYPE_SHAPE_MASK:
        case FFL_MODULATE_TYPE_SHAPE_NOSELINE:
        case FFL_MODULATE_TYPE_SHAPE_GLASS:
            mShader.setUniform(true, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_ALPHA_TEST]);
        default:
            mShader.setUniform(false, u32(-1), mPixelUniformLocation[PIXEL_UNIFORM_ALPHA_TEST]);
            break;
    }
}

void ShaderMiitomo::bindBodyShader(bool light_enable, FFLiCharInfo* pCharInfo)
{
    if (!light_enable) return mpMaskShader->bindBodyShader(light_enable, pCharInfo);
    bind(light_enable, pCharInfo);
    //RIO_GL_CALL(glBindVertexArray(vao));

    setCulling(FFL_CULL_MODE_BACK);

    // set body uniforms

    const FFLColor& favoriteColor = FFLGetFavoriteColor(pCharInfo->favoriteColor);

    const FFLModulateType modulateType = static_cast<FFLModulateType>(9);
    const FFLModulateParam modulateParam = {
        FFL_MODULATE_MODE_0,
        // CUSTOM MODULATE TYPE FOR setModulate
        modulateType,
        &favoriteColor,
        nullptr, // no color G
        nullptr, // no color B
        nullptr  // no texture
    };
    setModulate_(modulateParam);
    setMaterial_(modulateType);
}

void ShaderMiitomo::draw_(const FFLDrawParam& draw_param)
{
    setCulling(draw_param.cullMode);
    setModulate_(draw_param.modulateParam);
    setMaterial_(draw_param.modulateParam.type);

    if (draw_param.primitiveParam.pIndexBuffer != nullptr)
    {
#if RIO_IS_CAFE
        for (int type = FFL_ATTRIBUTE_BUFFER_TYPE_POSITION; type <= FFL_ATTRIBUTE_BUFFER_TYPE_COLOR; ++type)
        {
            GX2SetAttribBuffer(
                type,
                draw_param.attributeBufferParam.attributeBuffers[type].size,
                draw_param.attributeBufferParam.attributeBuffers[type].stride,
                draw_param.attributeBufferParam.attributeBuffers[type].ptr
            );
        }
#elif RIO_IS_WIN

        GLuint indexBufferHandle;
        glGenBuffers(1, &indexBufferHandle);  // Generate a new buffer

        for (int type = FFL_ATTRIBUTE_BUFFER_TYPE_POSITION; type <= FFL_ATTRIBUTE_BUFFER_TYPE_COLOR; ++type)
        {
            const FFLAttributeBuffer& buffer = draw_param.attributeBufferParam.attributeBuffers[type];
            s32 location = mAttributeLocation[type];
            void* ptr = buffer.ptr;

            if (ptr && location != -1 && buffer.stride > 0)
            {
                u32 stride = buffer.stride;
                u32 vbo_handle = mVBOHandle[type];
                u32 size = buffer.size;

                // Bind buffer and set vertex attribute pointer
                RIO_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_handle));
                RIO_GL_CALL(glBufferData(GL_ARRAY_BUFFER, size, ptr, GL_STATIC_DRAW));
                RIO_GL_CALL(glEnableVertexAttribArray(location));

                // Determine attribute pointer parameters based on buffer type
                switch (type)
                {
                case FFL_ATTRIBUTE_BUFFER_TYPE_POSITION:
                    RIO_GL_CALL(glVertexAttribPointer(location, 3, GL_FLOAT, false, stride, nullptr));
                    break;
                case FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD:
                    RIO_GL_CALL(glVertexAttribPointer(location, 2, GL_FLOAT, false, stride, nullptr));
                    break;
                case FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL:
                    RIO_GL_CALL(glVertexAttribPointer(location, 4, GL_INT_2_10_10_10_REV, true, stride, nullptr));
                    break;
                case FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT:
                    RIO_GL_CALL(glVertexAttribPointer(location, 4, GL_BYTE, true, stride, nullptr));
                    break;
                case FFL_ATTRIBUTE_BUFFER_TYPE_COLOR:
                    RIO_GL_CALL(glVertexAttribPointer(location, 4, GL_UNSIGNED_BYTE, true, stride, nullptr));
                    break;
                default:
                    break;
                }
            }
            else if (location != -1)
                // Disable the attribute to avoid using uninitialized data
                RIO_GL_CALL(glDisableVertexAttribArray(location));

        }
#endif

        // Draw elements
        rio::Drawer::DrawElements(
            draw_param.primitiveParam.primitiveType,
            draw_param.primitiveParam.indexCount,
            (const u16*)draw_param.primitiveParam.pIndexBuffer
        );
    }
}


void ShaderMiitomo::drawCallback_(void* p_obj, const FFLDrawParam* draw_param)
{
    static_cast<ShaderMiitomo*>(p_obj)->draw_(*draw_param);
}

void ShaderMiitomo::setMatrix_(const rio::BaseMtx44f& matrix)
{
    mShader.setUniform(matrix, mVertexUniformLocation[VERTEX_UNIFORM_MVP], u32(-1));
}

void ShaderMiitomo::setMatrixCallback_(void* p_obj, const rio::BaseMtx44f* matrix)
{
    static_cast<ShaderMiitomo*>(p_obj)->setMatrix_(*matrix);
}
