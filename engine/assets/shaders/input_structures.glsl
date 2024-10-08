
layout (set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} sceneData;

layout (set = 1, binding = 0) uniform GLTFMaterialData {
    vec4 colorFactors;
    vec4 emissiveFactors;
    float normalScale;
    float occlusionStrength;
    vec4 metallicRoughnessFactors;
} materialData;

struct Material
{
    vec4 albedo;
    vec4 emissive;
    float roughness;
    float metallic;
    float occlusion;
    vec3 f0;
    vec3 f90;
    vec3 diffuse;
};

struct Light
{
    vec3 direction;
    float attenuation;
    vec3 color;
    float intensity;
};

layout (set = 1, binding = 1) uniform sampler2D colorTexture;
layout (set = 1, binding = 2) uniform sampler2D emissiveTexture;
layout (set = 1, binding = 3) uniform sampler2D normalTexture;
layout (set = 1, binding = 4) uniform sampler2D occlusionTexture;
layout (set = 1, binding = 5) uniform sampler2D metallicRoughnessTexture;