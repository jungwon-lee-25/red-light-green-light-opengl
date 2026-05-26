#version 430 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 normal;

out vec3 vsPos;
out vec3 vsNormal;
out vec2 vsTex;

uniform mat4 model, view, projection;
uniform int isModel;

void main() {
    vsPos = vec3(model * vec4(pos, 1.0));
    vsNormal = normalize(mat3(transpose(inverse(model))) * (isModel == 0 ? -normal : normal));
    vsTex = tex;
    gl_Position = projection * view * vec4(vsPos, 1.0);
}