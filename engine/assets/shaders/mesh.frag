#version 460

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPosition;

layout (location = 0) out vec4 outFragColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    Material mat;
    mat.albedo = pow(texture(colorTexture, inUV), vec4(2.2) * materialData.colorFactors);
    mat.occlusion = texture(metallicRoughnessTexture, inUV).x * materialData.occlusionStrength;
    mat.emissive = texture(emissiveTexture, inUV) * materialData.emissiveFactors;
    mat.roughness = texture(metallicRoughnessTexture, inUV).y * materialData.metallicRoughnessFactors.x;
    mat.metallic = texture(metallicRoughnessTexture, inUV).z * materialData.metallicRoughnessFactors.y;
    
    vec3 N = normalize(inNormal);
    vec3 V = normalize(inverse(sceneData.view)[3].xyz * vec3(0.0, 0.0, 1.0) - inPosition).xyz;
    
    mat.f0 = mix(vec3(0.04), mat.albedo.xyz, mat.metallic);
    vec3 Lo = vec3(0.0);
    
    for (int i = 0; i < 1 ; i++) 
    {
        vec3 L = normalize(sceneData.sunlightDirection.xyz - inPosition);
        vec3 H = normalize(V + L);
        float distance = length(sceneData.sunlightDirection.xyz - inPosition);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = sceneData.sunlightColor.xyz * attenuation;
        
        float NDF = DistributionGGX(N, H, mat.roughness);
        float G = GeometrySmith(N, V, L, mat.roughness);
        vec3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), mat.f0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - mat.metallic;
        
        float NdotL = max(dot(N, L), 0.0);
        
        Lo += (kD * mat.albedo.rgb / PI + specular) * radiance * NdotL;
    }
    
    vec3 metallic = vec3(mat.metallic);
    vec3 roughness = vec3(mat.roughness);
    vec3 occlusion = vec3(mat.occlusion);
    vec3 albedo = mat.albedo.rgb;
    vec3 emissive = mat.emissive.rgb;
        
    vec3 color = (mat.albedo.rgb + Lo + mat.emissive.rgb) * mat.occlusion;
    
    //color = color / (color + vec3(1.0));
    //color = pow(color, vec3(1.0/2.2));    
    
    outFragColor = vec4(color, 1.0f);
}