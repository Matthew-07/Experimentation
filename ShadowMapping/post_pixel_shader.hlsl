#include "shared.hlsli"

float4 main(PostPSInput pin) : SV_TARGET
{
	return postTexture.Sample(postSampler, pin.tex);
}