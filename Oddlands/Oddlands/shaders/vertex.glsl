#version 330 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vColor;
layout (location = 2) in vec3 vNormal;

out vec3 color;
out vec3 worldNormal;

uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;

void main()
{
    gl_Position =
        projectionTransform *
        viewTransform *
        modelTransform *
        vec4(vPos, 1.0f);

    color = vColor;
    // [추가] 회전·가로세로 크기 변경 후에도 법선이 면에 수직이 되도록 역행렬의 전치를 사용한다.
    worldNormal = transpose(inverse(mat3(modelTransform))) * vNormal;
}
