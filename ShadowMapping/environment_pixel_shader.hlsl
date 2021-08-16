#include "shared.hlsli"

float4 main(PSInput pin) : SV_TARGET
{
	return environmentTexture.Sample(diffuseSampler, pin.worldPosition);
}