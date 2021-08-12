struct VS_Input
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VS_Output
{
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 position : SV_POSITION;
};

struct PS_Input
{
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;  
};

struct LightSource
{
    float4 position;
    float3 color;
    float intensity;
};

cbuffer ObjectConstantBuffer : register(b0)
{
    float4x4 model_matrix; // Needed for world positions
    float4x4 mvp_matrix;
    float4x4 mvp_normalMatrix;
    float k_s;
    float alpha;
    //float2 padding;
}

cbuffer SceneConstantBuffer : register(b1)
{
    float4 camera_position;
    LightSource lightSources[3];
    float3 ambient_color;
    float ambient_intensity;
}

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);