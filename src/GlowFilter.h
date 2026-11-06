#pragma once

#include "LayerFilter.h"
#include <array>

class GlowFilter : public LayerFilter {
public:
	LayerType getType() const override;

	void setup(int width, int height) override;
	void markDirty() override;
	void draw(ofFbo & source, float time) override;

	float getSigma() const;
	float getStrength() const;
	float getPeriod() const;

	void setParameters(float sigma, float strength, float period);

	static constexpr int MAX_BLUR_RADIUS = 32;

	struct BlurKernel {
		int radius = 0;

		// 中心からの距離0～32に対応する重み
		std::array<float, MAX_BLUR_RADIUS + 1> weights {};
	};

	BlurKernel makeBlurKernel(float sigma) const;

private:
	// 各解像度で、横方向にぼかした中間画像
	std::array<ofFbo, 4> preGlowFbos;

	// 各解像度で、横・縦のぼかしを終えた画像
	std::array<ofFbo, 4> glowFbos;

	// ぼかした画像のRGBに倍率を掛けて表示する
	ofShader compositeShader;

	ofShader glowShader;

	bool glowNeedupdate = true;

	// 減衰関数 I(r) = 1 / (1 + (r/a)^2) のa。
	// 各ガウシアンのsigmaは、この値にsigmaRatiosを掛けて求める。
	float glowSigma = 2.5f;
	float glowStrength = 4.5f;
	float glowPeriod = 1.0f;

	void blurInto(
		ofFbo & source,
		ofFbo & temporary,
		ofFbo & destination,
		float sigma,
		float strength);

	// ガウシアンを掛ける前の縮小画像
	// 順に、元画像の1/2、1/4、1/8
	std::array<ofFbo, 3> downsampleFbos;

	// 2×2テクセルの平均で縮小するシェーダー
	ofShader downsampleShader;

	void downsampleInto(ofFbo & source, ofFbo & destination);
};
