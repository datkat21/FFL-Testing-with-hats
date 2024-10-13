#version 330 core

#define VARYING_QUALIFIER in
#define VARYING_INSTANCE In

VARYING_QUALIFIER vec2 texCoord;

/// ================================================================
/// ピクセルシェーダーの実装
/// ================================================================

layout( location = 0 ) out vec4 o_Color;

//layout( std140 ) uniform u_Param
//{
    uniform vec2 u_OneDivisionResolution;
//};

uniform sampler2D s_Tex;

void main()
{
    
    float offsetX = u_OneDivisionResolution.x;
    float offsetY = u_OneDivisionResolution.y;

    /// アルファが設定されていれば何もしない。
    vec4 center = texture(s_Tex,texCoord);
    if(center.a > 0.0f)
    {
        o_Color = center;
        return;
    }
    
    
    /// 隣接ピクセルを取得する
    /// 012
    /// 3 4
    /// 567
    vec4 abutmentColor[8];
    abutmentColor[0] = texture(s_Tex,texCoord + vec2(-offsetX,-offsetY));
    abutmentColor[1] = texture(s_Tex,texCoord + vec2(    0.0f,-offsetY));
    abutmentColor[2] = texture(s_Tex,texCoord + vec2( offsetX,-offsetY));
    abutmentColor[3] = texture(s_Tex,texCoord + vec2(-offsetX,    0.0f));
    abutmentColor[4] = texture(s_Tex,texCoord + vec2( offsetX,    0.0f));
    abutmentColor[5] = texture(s_Tex,texCoord + vec2(-offsetX, offsetY));
    abutmentColor[6] = texture(s_Tex,texCoord + vec2(    0.0f, offsetY));
    abutmentColor[7] = texture(s_Tex,texCoord + vec2( offsetX, offsetY));

    
    vec3 sumColor = vec3(0);
    float count = 0.0f;
    for(int i = 0; i < 8;++i)
    {
        if(abutmentColor[i].a > 0.0f)
        {
            sumColor += abutmentColor[i].rgb;
            count += 1.0f;
        }
    }

    /// 隣接点のアルファが1つでも有効なら取得した要素で平均する
    if(count > 0.0f)
    {
        sumColor /= count;
    }
    else
    {
        /// 隣接がすべて無効ならcenter値を保持する
        sumColor = center.rgb;
    }
    
    
    o_Color = vec4(sumColor,0.0f);
}
