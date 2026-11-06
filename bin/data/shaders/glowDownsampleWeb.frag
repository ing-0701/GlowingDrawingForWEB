#version 300 es

precision highp float;

uniform sampler2D sourceTexture;

// 縮小元の画像サイズ
uniform vec2 imageSize;

in vec2 textCoord;

out vec4 outputColor;

void main() {
    // Web版では座標が0〜1なので、
    // 半テクセル分を、その座標系に変換する
    vec2 halfTexel = vec2(0.5) / imageSize;

    vec4 topLeft = texture(
        sourceTexture,
        textCoord + vec2(-halfTexel.x, -halfTexel.y)
    );

    vec4 topRight = texture(
        sourceTexture,
        textCoord + vec2(halfTexel.x, -halfTexel.y)
    );

    vec4 bottomLeft = texture(
        sourceTexture,
        textCoord + vec2(-halfTexel.x, halfTexel.y)
    );

    vec4 bottomRight = texture(
        sourceTexture,
        textCoord + vec2(halfTexel.x, halfTexel.y)
    );

    outputColor = (
        topLeft +
        topRight +
        bottomLeft +
        bottomRight
    ) * 0.25;
}
