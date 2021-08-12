#include "shared.hlsli"

float4 main(PSInput pin) : SV_TARGET
{
	float3 diffuse_color = diffuseTexture.Sample(diffuseSampler, pin.tex);

	float4 look_direction = float4(normalize(pin.worldPosition.xyz - camera_position.xyz), 0.f);

	float3 final_color = ambient.color * ambient.intensity * diffuse_color;

	for (uint i = 0; i < number_of_lights; ++i) {
		float shadowFactor = CalculateShadowFactor(pin.shadowPos[i], i);

		if (shadowFactor > 0.f) {
			// Compute vectors
			float3 lightDirection = point_lights[i].worldPosition - pin.worldPosition;
			float distance2 = pow(length(lightDirection), 2.f);
			lightDirection = normalize(lightDirection);

			float3 lightReflection = reflect(lightDirection, pin.normal);

			// Diffuse component

		}
	}

	return float4(final_color, 1.f);
}