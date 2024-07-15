#version 450 
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outTexCoord;

struct Vertex
{
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout (push_constant) uniform constants
{
    mat4 renderMatrix;
    VertexBuffer vertexBuffer;
} PushConstants;

void main() 
{
    Vertex vertex = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    
    gl_Position = PushConstants.renderMatrix * vec4(vertex.position, 1.0);
    outColor = vertex.color.rgb;
    outTexCoord.x = vertex.uv_x;
    outTexCoord.y = vertex.uv_y;
}