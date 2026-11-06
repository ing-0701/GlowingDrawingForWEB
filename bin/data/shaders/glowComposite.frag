#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect sourceTexture;
uniform float gain;

varying vec2 textCoord;

void main() {
    vec4 color = texture2DRect(sourceTexture, textCoord);

    gl_FragColor = vec4(
        color.rgb * gain,
        color.a
    );
}
