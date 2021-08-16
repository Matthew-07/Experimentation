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

	float a = 0.6f; // brightness
	float b = 2.3f; // contrast
	float gamma = 2.2f;

	color /= (float)sampleCount;

	// Tone mapping
	color = pow(color, b);
	color = color / (color + pow(0.5 / a, b));

	// Display encoding
	color = pow(color, 1.f / gamma);
	return float4(color, 1.f);
}