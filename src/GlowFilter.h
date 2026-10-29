#pragma once

#include "LayerFilter.h"

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

private:
	ofFbo preGlowFbo; // フィルタをかけるときに使う中間キャンバス
	ofFbo glowFbo1; // 実際に描画するもの１
	ofFbo glowFbo2; // 実際に描画するもの２

	ofShader glowShader;

	bool glowNeedupdate = true;

	float glowSigma = 1.5f;
	float glowStrength = 2.0f;
	float glowPeriod = 1.0f;

	void blurInto(
		ofFbo & source,
		ofFbo & destination,
		float sigma,
		float strength);
};
