#pragma once

#include "LayerFilter.h"

class GlowFilter : public LayerFilter {
public:
	LayerType getType() const override;

	void setup(int width, int height) override;
	void markDirty() override;
	void draw(ofFbo & source, float time) override;

private:
	ofFbo preGlowFbo; // フィルタをかけるときに使う中間キャンバス
	ofFbo glowFbo1; // 実際に描画するもの１
	ofFbo glowFbo2; // 実際に描画するもの２

	ofShader glowShader;

	bool glowNeedupdate = true;
	float glowStrength = 2.0f;

	void blurInto(
		ofFbo & source,
		ofFbo & destination,
		float sigma,
		float strength);
};
