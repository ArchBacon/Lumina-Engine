
layout (set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} sceneData;

layout (set = 1, binding = 0) uniform GLTFMaterialData {
    vec4 colorFactors;
    vec4 metallicRoughnessFactors;
} materialData;

layout (set = 1, binding = 1) uniform sampler2D colorTexture;
layout (set = 1, binding = 2) uniform sampler2D matellicRoughnessTexture;