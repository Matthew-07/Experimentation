#include "shared.hlsli"

PSInput main(VSInput vin) {
	PSInput vout;
	
	vout.worldPosition = mul(model_matrix, float4(vin.position, 1.f)).xyz;
	vout.position = mul(view_proj_matrix, float4(vout.worldPosition, 1.f));

	vout.normal = mul(normal_matrix, float4(vin.normal, 0.f)).xyz;

	vout.tex = vin.tex;

	//for (uint i = 0; i < number_of_lights; ++i) {
	//	vout.shadowPos[i] = mul(point_lights[i].shadowTransform, float4(vin.position, 1.f));
	//}
	
	return vout;
}