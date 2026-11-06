#version 300 es

precision highp float;
precision highp int;

uniform sampler2D sourceTexture;

uniform vec2 direction;
uniform vec2 imageSize;
uniform float strength;

const int MAX_BLUR_RADIUS = 32;

uniform int radius;
uniform float weights[MAX_BLUR_RADIUS + 1];

in vec2 textCoord;    //補間された後の、元画像の座標

out vec4 outputColor;

vec4 readSource(vec2 pixelPos) {
    if (
        pixelPos.x < 0.0 ||
        pixelPos.x >= imageSize.x ||
        pixelPos.y < 0.0 ||
        pixelPos.y >= imageSize.y
    ) {
        return vec4(0.0);
    }

    return texture(sourceTexture, pixelPos / imageSize);
}

void main() {
    // WebGLの0〜1の座標をテクセル単位にする
    vec2 pixelPos = textCoord * imageSize;

	vec4 totalColor = readSource(pixelPos) * weights[0];

    for (int i = 1; i <= MAX_BLUR_RADIUS; ++i) {
        if (i > radius) {
            break;
        }

        vec2 offset = direction * float(i);

        totalColor += (
            readSource(pixelPos - offset) +
            readSource(pixelPos + offset)
        ) * weights[i];
    }

	outputColor = vec4(
		totalColor.rgb * strength,
		totalColor.a
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
