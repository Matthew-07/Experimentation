#include "shared.hlsli"

float4 main(float3 pos : POSITION) : SV_POSITION
{
	float4 output = mul(model_matrix, float4(pos, 1.f));
	output = mul(view_proj_matrix, output);
	return output;
}