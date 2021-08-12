#include "shared.hlsli"

VS_Output main(VS_Input input)
{
    VS_Output output;
    output.worldPosition = mul(model_matrix, float4(input.position, 1.f));
    output.position = mul(mvp_matrix, float4(input.position, 1.f));
    output.normal = mul(mvp_normalMatrix, float4(input.normal, 0.f)).xyz;
    output.uv = input.uv;
    
    return output;
    
}