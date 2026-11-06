#include "GlowFilter.h"

#include <cmath>
#include <algorithm>

LayerType GlowFilter::getType() const {
	return LayerType::Glow;
}

void GlowFilter::setup(int width, int height) {
	ofFbo::Settings settings;
	settings.internalformat = GL_RGBA16F;

	settings.useDepth = false;
	settings.useStencil = false;
	settings.numSamples = 0;

	settings.minFilter = GL_LINEAR;
	settings.maxFilter = GL_LINEAR;

	settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
	settings.wrapModeVertical = GL_CLAMP_TO_EDGE;
#ifdef __EMSCRIPTEN__
	settings.textureTarget = GL_TEXTURE_2D;
#else
	settings.textureTarget = GL_TEXTURE_RECTANGLE_ARB;
#endif

    // 原寸、1/2、1/4、1/8
	int levelWidth = width;
	int levelHeight = height;

	for (int i = 0; i < 4; ++i) {
		settings.width = levelWidth;
		settings.height = levelHeight;

		preGlowFbos[i].allocate(settings);
		glowFbos[i].allocate(settings);

		// 原寸はsourceを直接使うため、縮小元FBOは不要
		if (i > 0) {
			downsampleFbos[i - 1].allocate(settings);
		}

		levelWidth /= 2;
		levelHeight /= 2;
	}

#ifdef __EMSCRIPTEN__
	const bool blurLoaded = glowShader.load("shaders/glowBlurWeb");

	const bool downsampleLoaded = downsampleShader.load("shaders/glowDownsampleWeb");

	const bool compositeLoaded = compositeShader.load("shaders/glowCompositeWeb");
#else
	const bool blurLoaded = glowShader.load("shaders/glowBlur");

	const bool downsampleLoaded = downsampleShader.load("shaders/glowDownsample");

	const bool compositeLoaded = compositeShader.load("shaders/glowComposite");
#endif

    if (!blurLoaded) {
		ofLogError("GlowFilter")
			<< "Failed to load glow shader";
	}

	if (!downsampleLoaded) {
		ofLogError("GlowFilter")
			<< "Failed to load downsample shader";
	}

	if (!compositeLoaded) {
		ofLogError("GlowFilter")
			<< "Failed to load composite shader";
	}

	// 確保した画像へ、次回drawで内容を書き込む
	glowNeedupdate = true;
}

void GlowFilter::markDirty() {
	glowNeedupdate = true;
}

void GlowFilter::draw(ofFbo & source, float time) {
	if (
		!glowShader.isLoaded() || !downsampleShader.isLoaded() || !compositeShader.isLoaded()) {
		return;
	}

	// ここから下把握できてない！

	// 元画像換算のsigmaと、aの比率
	constexpr float sigmaRatios[4] = {
		0.628f,
		1.444f,
		3.355f,
		9.410f
	};

	// 各画像の縮小倍率の分母
	constexpr float scaleDivisors[4] = {
		1.0f,
		2.0f,
		4.0f,
		8.0f
	};

	    // 正規化済みガウシアンを合成するための初期係数
	// w_i * sigma_i^2に比例し、合計を1にしている
	constexpr float blendWeights[4] = {
		0.07575949f,
		0.21115654f,
		0.26801263f,
		0.44507133f
	};
	// ここまで！！

	// 元画像が変化したときだけ改めてフィルターをかける
	if (glowNeedupdate) {
		// ぼかす前の縮小画像を作る
		downsampleInto(source, downsampleFbos[0]);

		downsampleInto(
			downsampleFbos[0],
			downsampleFbos[1]);

		downsampleInto(
			downsampleFbos[1],
			downsampleFbos[2]);

		// 4種類の解像度で、それぞれぼかす
		for (int i = 0; i < 4; ++i) {
			ofFbo & input = (i == 0)
				? source
				: downsampleFbos[i - 1];

			const float localSigma = glowSigma * sigmaRatios[i] / scaleDivisors[i];

			blurInto(
				input,
				preGlowFbos[i],
				glowFbos[i],
				localSigma,
				1.0f);
		}

		glowNeedupdate = false;
	}

	const float phase = TWO_PI * time / glowPeriod;
	const float amount = (1.0f - std::cos(phase)) * 0.5f;
	const float minBrightness = 0.25f;
	const float maxBrightness = 1.0f;
	const float brightness = minBrightness + (maxBrightness - minBrightness) * amount;

	ofPushStyle();

	ofEnableAlphaBlending();
	ofSetColor(255, 255, 255, 255);

	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(
		GL_ONE, GL_ONE,
		GL_ZERO, GL_ONE);


	// ここから下把握できてない！
	compositeShader.begin();

	for (int i = 0; i < 4; ++i) {
		// 最も狭い光は一定、それ以外の光を点滅させる
		const float pulse = (i == 0) ? 1.0f : brightness;

		const float gain = glowStrength * blendWeights[i] * pulse;

		compositeShader.setUniformTexture(
			"sourceTexture",
			glowFbos[i].getTexture(),
			0);

		compositeShader.setUniform1f("gain", gain);

		// 低解像度の画像も、元のキャンバス全体へ拡大する
		glowFbos[i].draw(
			0,
			0,
			source.getWidth(),
			source.getHeight());
	}

	compositeShader.end();

	ofPopStyle();

	// 次のレイヤー描画に向け、従来の設定へ戻す
	glBlendEquation(GL_FUNC_ADD);

	glBlendFuncSeparate(
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// ここまで
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
	sigma = std::clamp(sigma, 1.2f, 4.8f);
	strength = std::clamp(strength, 1.5f, 6.0f);
	period = std::clamp(period, 0.3f, 3.0f);

	// ぼかし画像の作り直しが必要か調べる
	const bool needsBlurUpdate = glowSigma != sigma;

	glowSigma = sigma;
	glowStrength = strength;
	glowPeriod = period;

	if (needsBlurUpdate) {
		markDirty();
	}
}

void GlowFilter::blurInto(ofFbo & source, ofFbo & temporary, ofFbo & destination, float sigma, float strength) {
	if (!std::isfinite(sigma) || sigma <= 0.0f) {
		ofLogError("GlowFilter")
			<< "Invalid blur sigma: " << sigma;
		return;
	}

	const int requiredRadius = static_cast<int>(std::ceil(3.0f * sigma));

	if (requiredRadius > MAX_BLUR_RADIUS) {
		ofLogError("GlowFilter")
			<< "Blur radius exceeds the supported maximum: "
			<< requiredRadius;
		return;
	}

	const BlurKernel kernel = makeBlurKernel(sigma);

	ofPushStyle();

	// 1回目：元画像を横方向にぼかす
	temporary.begin();
	ofClear(0, 0, 0, 0);
	ofDisableAlphaBlending();
	ofSetColor(255, 255, 255, 255);

	glowShader.begin();
	glowShader.setUniformTexture("sourceTexture", source.getTexture(), 0);
	glowShader.setUniform2f("direction", 1.0f, 0.0f);
	glowShader.setUniform2f("imageSize", source.getWidth(), source.getHeight());
	glowShader.setUniform1i("radius", kernel.radius);
	glowShader.setUniform1fv(
		"weights",
		kernel.weights.data(),
		MAX_BLUR_RADIUS + 1);
	glowShader.setUniform1f("strength", 1.0f);

	source.draw(0, 0);
	glowShader.end();
	temporary.end();

	// 2回目：横方向の結果を縦方向にぼかす
	destination.begin();
	ofClear(0, 0, 0, 0);
	ofDisableAlphaBlending();
	ofSetColor(255, 255, 255, 255);

	glowShader.begin();
	glowShader.setUniformTexture("sourceTexture", temporary.getTexture(), 0);
	glowShader.setUniform2f("direction", 0.0f, 1.0f);
	glowShader.setUniform2f("imageSize", temporary.getWidth(), temporary.getHeight());
	glowShader.setUniform1i("radius", kernel.radius);
	glowShader.setUniform1fv(
		"weights",
		kernel.weights.data(),
		MAX_BLUR_RADIUS + 1);
	glowShader.setUniform1f("strength", strength);

	temporary.draw(0, 0);

	glowShader.end();
	destination.end();

	ofPopStyle();
}

GlowFilter::BlurKernel GlowFilter::makeBlurKernel(float sigma) const {
	BlurKernel kernel;

	// まずは±3σを使用する
	constexpr float cutofSigma = 3.0f;

	// 打ち切り範囲
	kernel.radius = static_cast<int>(std::ceil(cutofSigma * sigma));

	float totalWeight = 0.0f;

	for (int i = 0; i <= kernel.radius; ++i) {
		const float distance = static_cast<float>(i);

		const float weight = std::exp(
			-(distance * distance) / (2.0 * sigma * sigma));

		kernel.weights[i] = weight;

		if (i == 0) {
			totalWeight += weight;
		} else {
			totalWeight += weight * 2.0f;
		}
	}

	for (int i = 0; i <= kernel.radius; ++i) {
		kernel.weights[i] /= totalWeight;
	}

	return kernel;
}

void GlowFilter::downsampleInto(ofFbo& source, ofFbo& destination) {
	ofPushStyle();

	destination.begin();

	ofClear(0, 0, 0, 0);

	// 計算したRGBAを、そのまま保存する
	ofDisableAlphaBlending();
	ofSetColor(255, 255, 255, 255);

	downsampleShader.begin();

	downsampleShader.setUniformTexture(
		"sourceTexture",
		source.getTexture(),
		0
	);

#ifdef __EMSCRIPTEN__
	downsampleShader.setUniform2f(
		"imageSize",
		source.getWidth(),
		source.getHeight()
	);
#endif // __EMSCRIPTEN__

	    // 元画像全体を、縮小先のサイズに描く
	source.draw(
		0,
		0,
		destination.getWidth(),
		destination.getHeight());

	downsampleShader.end();

	destination.end();

	ofPopStyle();
}
