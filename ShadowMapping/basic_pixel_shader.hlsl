#include "shared.hlsli"

float4 main(PSInput pin) : SV_TARGET
{
	//return float4(1.f, 0.f, 0.f, 1.f);

	float3 diffuse_color = diffuseTexture.Sample(diffuseSampler, pin.tex).xyz;

	float4 look_direction = float4(normalize(pin.worldPosition.xyz - camera_position.xyz), 0.f);

	float3 final_color = ambient.color * ambient.intensity * diffuse_color;

	for (uint i = 0; i < number_of_lights; ++i) {
		//float shadowFactor = CalculateShadowFactor(pin.shadowPos[i], i);

		float3 lightDirection = point_lights[i].worldPosition - pin.worldPosition;
		float shadowFactor = CalculateShadowFactorCube(-lightDirection, i);
		//float shadowFactor = 1.f;

		if (shadowFactor > 0.f) {
			// Compute vectors
			float distance2 = pow(length(lightDirection), 2.f);
			lightDirection = normalize(lightDirection);

			float3 lightReflection = reflect(lightDirection, pin.normal);

			// Diffuse component
			final_color.xyz += (diffuse_color * point_lights[i].color * point_lights[i].intensity * max(dot(lightDirection.xyz, pin.normal), 0.f)) / distance2 * shadowFactor;

			// Specular component
			final_color.xyz += (point_lights[i].color * point_lights[i].intensity * k_s * pow(max(dot(lightReflection, look_direction.xyz), 0.f), alpha)) / distance2 * shadowFactor;
		}
	}

	return float4(final_color, 1.f);
}