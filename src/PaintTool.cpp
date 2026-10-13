#include "PaintTool.h"
#include <cmath>

PaintTool::PaintTool(int mainWidth, int mainHeight) {
	color = ofColor(0, 0, 0, 255);
	strokeSize = 0;

	ofFbo::Settings s;
	s.width = mainWidth;
	s.height = mainHeight;
	s.internalformat = GL_RGBA32F;

	fbo2D.allocate(s);
	preFbo2D.allocate(s);
	historySwapFbo.allocate(s);
	glowCoreFbo.allocate(s);
	preGlowFbo.allocate(s);
	glowFbo1.allocate(s);
	glowFbo2.allocate(s);

	for (int i = 0; i < UNDO_CAPACITY; i++) {
		undoFbos[i].allocate(s);
	}

	glowCoreFbo.begin();
	ofClear(0, 0, 0, 0);
	glowCoreFbo.end();

#ifdef __EMSCRIPTEN__
#else
#endif // __EMSCRIPTEN__
}

void PaintTool::drawLine(bool penSwitch, float eraserAlpha) {
	ofFbo & base = glowCanvas ? glowCoreFbo : fbo2D;

	ofFill();

	preFbo2D.begin();
	ofClear(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	base.draw(0, 0);

	if (penSwitch) {
		ofSetColor(color);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		ofSetColor(255, 255, 255, eraserAlpha);
		glBlendFuncSeparate(
			GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
			GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
	}

	for (int i = 0; i < points.size(); i++) {
		ofDrawCircle(points[i].x, points[i].y, strokeSize * 25);
	}

	preFbo2D.end();

	if (glowCanvas) {
		glowNeedUpdate = true;
	}
}

void PaintTool::drawCanvas(bool drawingFlag) {
	ofFbo & normalSource = (drawingFlag && !glowCanvas) ? preFbo2D : fbo2D;
	ofFbo & glowSource = (drawingFlag && glowCanvas) ? preFbo2D : glowCoreFbo;

#ifdef __EMSCRIPTEN__
#else
	const bool canDrawGlow = glowShader.isLoaded();

	if (canDrawGlow && glowNeedUpdate) {
		blurInto(glowSource, glowFbo1, 1.5f, 1.0f);
		blurInto(glowSource, glowFbo2, 4.0f, glowStrength);
	}

	glowNeedUpdate = false;
#endif // __EMSCRIPTEN__

	ofEnableAlphaBlending();
	ofSetColor(255, 255, 255, 255);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	normalSource.draw(0, 0);

#ifdef __EMSCRIPTEN__
	glowSource.draw(0, 0);
#else
	if (canDrawGlow) {
		glowFbo1.draw(0, 0);

		const float period = 3.0f;
		const float time = ofGetElapsedTimef();
		const float phase = TWO_PI * time / period;
		const float amount = (1.0f - std::cos(phase)) * 0.5f;

		const float minBrightness = 0.25f;
		const float maxBrightness = 1.0f;
		const float brightness = minBrightness + (maxBrightness - minBrightness) * amount;

		glBlendFuncSeparate(
			GL_ONE, GL_ONE,
			GL_ZERO, GL_ONE);

		ofSetColor(ofFloatColor(brightness, brightness, brightness, 1.0f));

		glowFbo2.draw(0, 0);

		ofSetColor(255, 255, 255, 255);

		glBlendFuncSeparate(
			GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
			GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}
#endif // __EMSCRIPTEN__
}

void PaintTool::copyFbo(ofFbo & source, ofFbo & destination) {
	ofPushStyle();

	destination.begin();
	ofClear(0, 0, 0, 0);
	ofDisableAlphaBlending();
	ofSetColor(255, 255, 255, 255);

	source.draw(0, 0);

	destination.end();

	ofPopStyle();
}

void PaintTool::swapHistory(int index) {
	ofFbo & target = (undoTargets[index] == 1) ? glowCoreFbo : fbo2D;

	copyFbo(target, historySwapFbo);
	copyFbo(undoFbos[index], target);
	copyFbo(historySwapFbo, undoFbos[index]);

	if (undoTargets[index] == 1) {
		glowNeedUpdate = true;
	}
}

void PaintTool::saveUndo() {
	ofFbo & target = glowCanvas ? glowCoreFbo : fbo2D;

	copyFbo(target, undoFbos[undoWriteIndex]);

	undoTargets[undoWriteIndex] = glowCanvas ? 1 : 0;

	undoWriteIndex = (undoWriteIndex + 1) % UNDO_CAPACITY;

	if (undoCount < UNDO_CAPACITY) {
		undoCount++;
	}
	redoCount = 0;
}

void PaintTool::undo() {
	if (undoCount <= 0) {
		return;
	}

	undoWriteIndex = (undoWriteIndex + UNDO_CAPACITY - 1) % UNDO_CAPACITY;

	swapHistory(undoWriteIndex);

	undoCount--;
	redoCount++;
}

void PaintTool::redo() {
	if (redoCount <= 0) {
		return;
	}

	swapHistory(undoWriteIndex);

	undoWriteIndex = (undoWriteIndex + 1) % UNDO_CAPACITY;

	undoCount++;
	redoCount--;
}

void PaintTool::deleteAll() {
	ofFbo & target = glowCanvas ? glowCoreFbo : fbo2D;

	saveUndo();

	target.begin();
	ofClear(0, 0, 0, 0);
	target.end();
}

#ifdef __EMSCRIPTEN__
#else
void PaintTool::setupGlow() {
	if (!glowShader.load("shaders/glowBlur")) {
		ofLogError("PaintTool") << "Failed to load glow shader";
	}

	if (glowCanvas) {
		glowNeedUpdate = true;
	}
}

void PaintTool::blurInto(ofFbo & source, ofFbo & destination, float sigma, float strength) {
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
#endif // __EMSCRIPTEN__
