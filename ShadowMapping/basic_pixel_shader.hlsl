#include "shared.hlsli"

float4 main(PSInput pin) : SV_TARGET
{
	//return float4(1.f, 0.f, 0.f, 1.f);

	float3 diffuse_color = diffuseTexture.Sample(diffuseSampler, pin.tex).xyz;

	float4 look_direction = float4(normalize(pin.worldPosition.xyz - camera_position.xyz), 0.f);

	float3 final_color = ambient.color * ambient.intensity * diffuse_color;

	for (uint i = 0; i < number_of_point_lights; ++i) {
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

	for (uint i = 0; i < number_of_directional_lights; ++i) {
		
		float shadowFactor = CalculateShadowFactor(pin.shadowPos[i], i);
		//float shadowFactor = 1.f;

		if (shadowFactor > 0.f) {
			float3 lightDirection = -directional_lights[i].direction;
			//lightDirection = normalize(lightDirection);

			// Compute vectors
			// We only care about distance along the lightVector
			float3 lightCenterDirection = directional_lights[i].position - pin.worldPosition;
			float distance = dot(lightCenterDirection, lightDirection);
			if (distance < 0.f) continue; // The light only shines in one direction
			//else distance2 = pow(distance2, 2.f);		

			// Check the pixel is within the cuboid containg the light
			//float3 horizontalDirection = normalize(cross(directional_lights[i].direction, directional_lights[i].upDirection));
			//if (abs(dot(lightCenterDirection, directional_lights[i].upDirection)) > directional_lights[i].height / 2.f
			//	|| abs(dot(lightCenterDirection, horizontalDirection)) > directional_lights[i].width / 2.f) continue;

			float3 lightReflection = reflect(lightDirection, pin.normal);

			// Diffuse component
			final_color.xyz += (diffuse_color * directional_lights[i].color * directional_lights[i].intensity * max(dot(lightDirection.xyz, pin.normal), 0.f)) * shadowFactor;

			// Specular component
			final_color.xyz += (directional_lights[i].color * directional_lights[i].intensity * k_s * pow(max(dot(lightReflection, look_direction.xyz), 0.f), alpha)) * shadowFactor;
		}
	}

	return float4(final_color, 1.f);
}