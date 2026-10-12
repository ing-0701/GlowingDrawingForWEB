#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect sourceTexture;
uniform vec2 direction;
uniform vec2 imageSize;
uniform float strength;

varying vec2 textCoord;    //補間された後の、元画像の座標

void main() {
	const float sigma = 4.0;    //sigmaが小さいほど遠くの画素が薄くなる(ぼけが強くなる)

	vec4 totalColor = vec4(0.0);
	float totalWeight = 0.0;

	for (int i = -24; i <= 12; ++i){
		float distance = float(i);

		float weight = exp(
			-(distance * distance) / (2.0 * sigma * sigma)
		);

		vec2 samplePos = textCoord + direction * distance;

		vec4 sampleColor = vec4(0.0);

		if(
			samplePos.x >= 0.0 &&
			samplePos.x < imageSize.x &&
			samplePos.y >= 0.0 &&
			samplePos.y < imageSize.y
		){
			sampleColor = texture2DRect(sourceTexture, samplePos);
		}

		totalColor += sampleColor * weight;
		totalWeight += weight;
	}

	vec4 blurredColor = totalColor / totalWeight;

	gl_FragColor = vec4(
		blurredColor.rgb * strength,
		blurredColor.a
	);
}

/*
ガウシアンフィルタによるぼかし。

各フラグメントについて、対応するテクスチャ座標を中心に、
前後12テクセル、合計25か所の色を読む。

各色には、中心に近いほど大きく、遠いほど小さい重みを付ける。
重みは次の式で求める。

    weight = exp(-distance² / (2 × sigma²))

sigmaは重みの広がりを表す値。今回は4.0。
大きくすると遠いテクセルの影響が強くなるが、
参照範囲の±12は自動では変わらない。

「色 × 重み」を合計し、重みの合計で割って、
各画素でぼかしたRGBAを求める。画像の外側は透明な黒として扱う。

directionが(1, 0)なら横方向、(0, 1)なら縦方向をぼかす。
C++側で「元画像 → 横ぼかし → 縦ぼかし」と2回適用することで、
縦横に広がるぼかしを、少ない参照回数で作る。

最後にRGBだけをstrength倍し、発光の強さを調整する。
元画像への加算合成は、このシェーダーではなくC++側で行う。
*/
