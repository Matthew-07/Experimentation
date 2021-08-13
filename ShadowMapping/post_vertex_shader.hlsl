#include "shared.hlsli"

PostPSInput main(PostVSInput vin)
{
	PostPSInput vout;
	vout.position = float4(vin.position, 1.f);
	vout.tex = vin.tex;

	return vout;
}