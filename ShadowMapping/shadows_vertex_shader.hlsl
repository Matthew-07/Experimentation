#include "shared.hlsli"

float4 main(float3 pos : POSITION) : SV_POSITION
{
	float4 output = mul(model_matrix, float4(pos, 1.f));
	output = mul(view_proj_matrix, output);
	//output.xyz /= output.w;
	//output.w = 1.f;
	//output.z = sqrt(pow(pos.x, 2.f) + pow(pos.y, 2.f) + pow(pos.z, 2.f));
	return output;
}