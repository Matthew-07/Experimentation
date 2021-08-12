struct PSInput
{
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
    float2 projTex : TEXCOORD1;
};

struct PointLight
{
    
};

cbuffer SceneConstantBuffer : register(b0)
{
    
}