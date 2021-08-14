#include "shared.hlsli"

float4 main(PostPSInput pin) : SV_TARGET
{
	//return postTexture.Sample(postSampler, pin.tex);

	float3 color = 0.f;

	uint width, height, sampleCount;
	postTexture.GetDimensions(width, height, sampleCount);

	for (uint i = 0; i < sampleCount; ++i) {
		color += postTexture.Load(pin.tex * float2(width, height) , i).xyz;
	}

	return float4(color / sampleCount, 1.f);
}