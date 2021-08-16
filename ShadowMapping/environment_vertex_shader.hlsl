#include "shared.hlsli"

PSInput main(VSInput vin)
{
	PSInput vout;
	vout.worldPosition = vin.position;// + camera_position;
	vout.position = mul(view_proj_matrix, float4(vin.position + camera_position, 1.f));
	vout.position.z = vout.position.w;

	return vout;
}