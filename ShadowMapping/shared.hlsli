// Textures
//texture2D postTexture : register(t0);
Texture2DMS<float4> postTexture : register(t0);
texture2D diffuseTexture : register(t1);
texture2D shadowMaps[3] : register(t2);
TextureCube shadowCubeMaps[3] : register(t5);

// Samplers
SamplerState postSampler : register(s0);
SamplerState diffuseSampler : register(s1);
SamplerComparisonState shadowSampler : register(s2);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;    
    //float4 shadowPos[3] : POSITION0;
    float3 worldPosition : POSITION3;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
};

struct PostVSInput {
    float3 position : POSITION;
    float2 tex : TEXCOORD;
};

struct PostPSInput {
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
};

struct DirectionalLight
{
    float4x4 shadowTransformation;
    float3 direction;
    float width;
    float3 color;
    float height;
    float3 position;
    float intensity;
};

struct PointLight
{
    float3 worldPosition;
    float3 color;
    float intensity;
};

struct AmbientLight
{
    float3 color;
    float intensity;
};

cbuffer objectCB : register(b0)
{
    float4x4 model_matrix;
    float4x4 normal_matrix;

    float k_s;
    float alpha;
}

cbuffer sceneCB : register(b1)
{
    float4x4 view_proj_matrix;

    float3 camera_position;

    AmbientLight ambient;

    PointLight point_lights[3];
    uint number_of_lights;    
}

float CalculateShadowFactor(float4 shadowPos, uint shadowMapIndex=0) {
    shadowPos.xyz /= shadowPos.w;

    float depth = shadowPos.z;

    //Initally, only sample one point in the shadow map (so the function will return 0.f or 1,f)
    float percentLit = 0.f;

    percentLit += shadowMaps[shadowMapIndex].SampleCmpLevelZero(shadowSampler, shadowPos.xy, depth).x;

    return percentLit;
}

float CalculateShadowFactorCube(float3 positionVector, uint shadowMapIndex = 0) {
    //shadowpos.xyz /= shadowpos.w;

    //float depth = 1.f / ((length(positionVector) - 0.1f) / 99.9f);
    //float depth = 1.f - (1.f / length(positionVector) / 10.f);
    //float depth = 1.f - ((1.f / length(positionVector)) - 0.1f) / 9.9f;

    float depth = max(max(abs(positionVector.x), abs(positionVector.y)), abs(positionVector.z));
    depth = 1.f - 1.f / depth / 10.f;

    //initally, only sample one point in the shadow map (so the function will return 0.f or 1,f)
    float percentlit = 0.f;

    percentlit += shadowCubeMaps[shadowMapIndex].SampleCmpLevelZero(shadowSampler, positionVector, depth).x;

    return percentlit;
}