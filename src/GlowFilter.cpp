#include "GlowFilter.h"

#include <cmath>
#include <algorithm>

LayerType GlowFilter::getType() const {
	return LayerType::Glow;
}

void GlowFilter::setup(int width, int height) {
	ofFbo::Settings settings;
	settings.width = width;
	settings.height = height;
	settings.internalformat = GL_RGBA16F;
#ifdef __EMSCRIPTEN__
	settings.textureTarget = GL_TEXTURE_2D;
#endif // __EMSCRIPTEN__

	preGlowFbo.allocate(settings);
	glowFbo1.allocate(settings);
	glowFbo2.allocate(settings);

#ifdef __EMSCRIPTEN__
	const bool loaded = glowShader.load("shaders/glowBlurWeb");
#else
	const bool loaded = glowShader.load("shaders/glowBlur");
#endif // __EMSCRIPTEN__

	if (!loaded) {
		ofLogError("GlowFilter") << "Failed to load glow shader";
	}
}

void GlowFilter::markDirty() {
	glowNeedupdate = true;
}

void GlowFilter::draw(ofFbo & source, float time) {
	if (!glowShader.isLoaded()) {
		return;
	}

	// 元画像が変化したときだけ改めてフィルターをかける
	if (glowNeedupdate) {
		blurInto(source, glowFbo1, glowSigma, 1.0f);
		blurInto(source, glowFbo2, glowSigma * (8.0f / 3.0f), glowStrength);

		glowNeedupdate = false;
	}

	const float phase = TWO_PI * time / glowPeriod;
	const float amount = (1.0f - std::cos(phase)) * 0.5f;
	const float minBrightness = 0.25f;
	const float maxBrightness = 1.0f;
	const float brightness = minBrightness + (maxBrightness - minBrightness) * amount;

	ofEnableAlphaBlending();
	ofSetColor(255, 255, 255, 255);

	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(
		GL_ONE, GL_ONE,
		GL_ZERO, GL_ONE);

	glowFbo1.draw(0, 0);

	ofSetColor(ofFloatColor(
		brightness, brightness, brightness, 1.0f));

	    glowFbo2.draw(0, 0);

	// 次の描画に向けて戻す
	ofSetColor(255, 255, 255, 255);

	glBlendFuncSeparate(
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

float GlowFilter::getSigma() const {
	return glowSigma;
}

float GlowFilter::getStrength() const{
	return glowStrength;
}

float GlowFilter::getPeriod() const {
	return glowPeriod;
}

void GlowFilter::setParameters(float sigma, float strength, float period) {
	sigma = std::clamp(sigma, 1.0f, 3.0f);
	strength = std::clamp(strength, 1.0f, 3.0f);
	period = std::clamp(period, 0.3f, 3.0f);

	// ぼかし画像の作り直しが必要か調べる
	const bool needsBlurUpdate = glowSigma != sigma || glowStrength != strength;

	glowSigma = sigma;
	glowStrength = strength;
	glowPeriod = period;

	if (needsBlurUpdate) {
		markDirty();
	}
}

void GlowFilter::blurInto(ofFbo & source, ofFbo & destination, float sigma, float strength) {
	ofPushStyle();

	// 1回目：元画像を横方向にぼかす
	preGlowFbo.begin();
	ofClear(0, 0, 0, 0);
	ofDisableAlphaBlending();
	ofSetColor(255, 255, 255, 255);

	glowShader.begin();
	glowShader.setUniformTexture("sourceTexture", source.getTexture(), 0);
	glowShader.setUniform2f("direction", 1.0f, 0.0f);
	glowShader.setUniform2f("imageSize", source.getWidth(), source.getHeight());
	glowShader.setUniform1f("sigma", sigma);
	glowShader.setUniform1f("strength", 1.0f);

	source.draw(0, 0);
	glowShader.end();
	preGlowFbo.end();

	// 2回目：横方向の結果を縦方向にぼかす
	destination.begin();
	ofClear(0, 0, 0, 0);
	ofDisableAlphaBlending();
	ofSetColor(255, 255, 255, 255);

	glowShader.begin();
	glowShader.setUniformTexture("sourceTexture", preGlowFbo.getTexture(), 0);
	glowShader.setUniform2f("direction", 0.0f, 1.0f);
	glowShader.setUniform2f("imageSize", preGlowFbo.getWidth(), preGlowFbo.getHeight());
	glowShader.setUniform1f("sigma", sigma);
	glowShader.setUniform1f("strength", strength);

	preGlowFbo.draw(0, 0);

	glowShader.end();
	destination.end();

	ofPopStyle();
}
