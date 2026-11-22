#version 330 core

in vec3 color;
out vec4 FragColor;
// [추가] HUD 반투명 배경과 한글 글자에만 사용하는 설정.
uniform float uiOpacity = 1.0;
uniform bool uiText = false;
uniform sampler2D uiTexture;
uniform vec3 uiTint;

void main()
{
    if (uiText)
        FragColor = vec4(uiTint, texture(uiTexture, color.xy).a * uiOpacity);
    else
        FragColor = vec4(color, uiOpacity);
}
