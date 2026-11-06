#version 300 es

precision highp float;

uniform sampler2D sourceTexture;
uniform float gain;

in vec2 textCoord;

out vec4 outputColor;

void main() {
    vec4 color = texture(sourceTexture, textCoord);

    outputColor = vec4(
        color.rgb * gain,
        color.a
    );
}
