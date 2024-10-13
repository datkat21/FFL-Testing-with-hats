#version 330 core

#define VARYING_QUALIFIER out
#define VARYING_INSTANCE Out

VARYING_QUALIFIER vec2 texCoord;

/// ================================================================
/// 頂点シェーダーの実装
/// ================================================================

layout( location = 0 ) in vec4 i_Position;
layout( location = 1 ) in vec2 i_TexCoord;

void main()
{
    texCoord = i_TexCoord.xy;
    gl_Position = i_Position;
    
}
