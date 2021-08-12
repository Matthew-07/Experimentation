#include "shared.hlsli"

float4 main(PS_Input input) : SV_TARGET
{        
    input.uv.y = 1.f - input.uv.y;
    
    float3 surface_color = g_texture.Sample(g_sampler, input.uv);
    
    float4 look_direction = float4(normalize(input.worldPosition.xyz - camera_position.xyz), 0.f);
    
    // Ambient
    float4 final_color = float4(ambient_color * ambient_intensity * surface_color, 1.f);
    
    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        float3 lightDirection = lightSources[i].position.xyz - input.worldPosition.xyz;        
        float distance2 = pow(length(lightDirection), 2.f);
        lightDirection = normalize(lightDirection);
        
        float3 lightReflection = reflect(lightDirection, input.normal);
        
        // Diffuse
        final_color.xyz += (surface_color * lightSources[i].color * lightSources[i].intensity * max(dot(lightDirection.xyz, input.normal), 0.f)) / distance2;

        // Specular
        final_color.xyz += (lightSources[i].color * lightSources[i].intensity * k_s * pow(max(dot(lightReflection, look_direction.xyz), 0.f), alpha)) / distance2;
        
        //if (i == 0)
        //{
        //    final_color.xyz = dot(lightReflection, look_direction.xyz);
        //    //final_color.xyz = lightReflection;
        //    //final_color.xyz = look_direction;
        //}
            
    }    
    
    return final_color;
}