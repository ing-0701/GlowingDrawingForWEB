#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect sourceTexture;

varying vec2 textCoord;

void main (){
	// 出力先の1ピクセルに対応する、元画像の中心位置
	vec2 center = textCoord;

	// 元画像の2×2テクセルの中心を読む
	vec4 topLeft = texture2DRect(
		sourceTexture,
		center + vec2(-0.5, -0.5)
	);

	    vec4 topRight = texture2DRect(
        sourceTexture,
        center + vec2(0.5, -0.5)
    );

    vec4 bottomLeft = texture2DRect(
        sourceTexture,
        center + vec2(-0.5, 0.5)
    );

    vec4 bottomRight = texture2DRect(
        sourceTexture,
        center + vec2(0.5, 0.5)
    );

    // RGBAすべてについて、4テクセルの平均を求める
	gl_FragColor = (
        topLeft +
        topRight +
        bottomLeft +
        bottomRight
    ) * 0.25;
}
