#version 330 core

in vec3 color;
in vec3 worldNormal;
out vec4 FragColor;
// [추가] HUD 반투명 배경과 한글 글자에만 사용하는 설정.
uniform float uiOpacity = 1.0;
uniform bool uiText = false;
uniform sampler2D uiTexture;
uniform vec3 uiTint;
// [추가] 홍범기 조명: 왼쪽 위의 따뜻한 햇빛 + 그늘에서도 보이도록 남기는 주변광.
uniform bool lightingEnabled = true;
uniform vec3 directionToSun = vec3(-0.6, 0.85, 0.45);
uniform vec3 sunColor = vec3(1.0, 0.96, 0.87);
uniform vec3 ambientColor = vec3(0.38, 0.40, 0.44);

void main()
{
    if (uiText)
        FragColor = vec4(uiTint, texture(uiTexture, color.xy).a * uiOpacity);
    else
    {
        vec3 surfaceColor = color;
        if (lightingEnabled)
        {
            // [추가] 빛을 향할수록 밝아지는 확산광. 반대쪽 면에는 주변광만 남긴다.
            float diffuse = max(dot(normalize(worldNormal), normalize(directionToSun)), 0.0);
            surfaceColor *= ambientColor + 0.75 * diffuse * sunColor;
        }
        FragColor = vec4(surfaceColor, uiOpacity);
    }
}
