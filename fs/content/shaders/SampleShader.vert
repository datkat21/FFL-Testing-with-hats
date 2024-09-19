#version 330 core

#define VARYING_QUALIFIER out
#define VARYING_INSTANCE Out

VARYING_QUALIFIER vec2 texCoord;
VARYING_QUALIFIER vec3 normal;
VARYING_QUALIFIER float specularMix;

/// ================================================================
/// 頂点シェーダーの実装
/// ================================================================

layout( location = 0 ) in vec4 i_Position;
// order of TexCoord and Normal flipped
// to be compliant with rio models
layout( location = 1 ) in vec2 i_TexCoord;
layout( location = 2 ) in vec3 i_Normal;
layout( location = 3 ) in vec4 i_Parameter;

/*
layout( location = 4 ) in vec4  i_Parameter;
layout( location = 5 ) in ivec4 i_Index;
layout(std140) uniform u_Skeleton
{
    vec4 mtxPalette[3 * 128];
};

layout(std140) uniform u_Shape
{
    vec4 shapeMtx[3];
    int vtxSkinCount;
};
*/

// TODO: you can use rio mShader.setUniformArray to bind this(?)
// https://github.com/aboood40091/RIO-Tests/blob/b9db5fb506c92d89c526d6c93efa0d44a7260510/07_Uniform-Variables-2/src/roottask.cpp#L193
/*
layout(std140) uniform u_Matrix
{
    vec4 mv[3];
    vec4 proj[4];
};


// vec4[3] * vec4(vec3, 1)
#define TRANSFORM_POS(mtx, pos) vec3( \
    dot(mtx[0].xyzw, pos.xyzw), \
    dot(mtx[1].xyzw, pos.xyzw), \
    dot(mtx[2].xyzw, pos.xyzw))

// vec3[3] * vec3
#define TRANSFORM_VEC(mtx, vec) vec3( \
    dot(mtx[0].xyz, vec.xyz), \
    dot(mtx[1].xyz, vec.xyz), \
    dot(mtx[2].xyz, vec.xyz))

// vec4[4] * vec4
#define PROJECT(mtx, pos) vec4( \
    dot(mtx[0].xyzw, pos.xyzw), \
    dot(mtx[1].xyzw, pos.xyzw), \
    dot(mtx[2].xyzw, pos.xyzw), \
    dot(mtx[3].xyzw, pos.xyzw))
*/

uniform mat4 mv;
uniform mat4 proj;

void main()
{
    /// ビュー行列に変換
    /*vec3 vPos = TRANSFORM_POS(mv,i_Position);
    gl_Position = PROJECT(proj,vec4(vPos,1.0f));
    */
    gl_Position = proj * mv * i_Position;
    normal = mat3(mv) * i_Normal.xyz;

    texCoord = i_TexCoord.xy;
    //normal = TRANSFORM_VEC(mv,i_Normal);
    specularMix = i_Parameter.r;


#if defined(USE_DEBUG)
    gl_Position = i_Position;
#endif
    
}
/*
    vec4 pos_w = vec4(0, 0, 0, 1);
    vec3 nrm_w = vec3(0, 0, 0);
    vec4 tmp = vec4(i_Position.xyz, 1.0f);
    
    if (vtxSkinCount == 0)
    {
        pos_w.x = dot(shapeMtx[0], tmp);
        pos_w.y = dot(shapeMtx[1], tmp);
        pos_w.z = dot(shapeMtx[2], tmp);
        nrm_w.x = dot(shapeMtx[0].xyz, i_Normal);
        nrm_w.y = dot(shapeMtx[1].xyz, i_Normal);
        nrm_w.z = dot(shapeMtx[2].xyz, i_Normal);
    }
    else if (vtxSkinCount == 1)
    {
        int mtxIndex = i_Index.x * 3;
        pos_w.x = dot(mtxPalette[mtxIndex + 0], tmp);
        pos_w.y = dot(mtxPalette[mtxIndex + 1], tmp);
        pos_w.z = dot(mtxPalette[mtxIndex + 2], tmp);
        nrm_w.x = dot(mtxPalette[mtxIndex + 0].xyz, i_Normal);
        nrm_w.y = dot(mtxPalette[mtxIndex + 1].xyz, i_Normal);
        nrm_w.z = dot(mtxPalette[mtxIndex + 2].xyz, i_Normal);
    }
    
    vec4 pos_v;
    pos_v.x = dot(cameraMtx[0], pos_w);
    pos_v.y = dot(cameraMtx[1], pos_w);
    pos_v.z = dot(cameraMtx[2], pos_w);
    pos_v.w = 1.0;

    gl_Position.x = dot(projMtx[0], pos_v);
    gl_Position.y = dot(projMtx[1], pos_v);
    gl_Position.z = dot(projMtx[2], pos_v);
    gl_Position.w = dot(projMtx[3], pos_v);
    
    Out.normal.x = dot(cameraMtx[0].xyz, nrm_w);
    Out.normal.y = dot(cameraMtx[1].xyz, nrm_w);
    Out.normal.z = dot(cameraMtx[2].xyz, nrm_w);
    Out.texCoord = i_TexCoord.xy;
    Out.specularMix = i_Parameter.r;
   
}
*/
