#include "PaintTool.h"

PaintTool::PaintTool(int mainWidth, int mainHeight) {
	color = ofColor(0, 0, 0, 255);
	strokeSize = 0;

	ofFbo::Settings s;
	s.width = mainWidth;
	s.height = mainHeight;
	s.internalformat = GL_RGBA32F;

	fbo2D.allocate(s);
	preFbo2D.allocate(s);
	redoFbo.allocate(s);
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
}

void PaintTool::drawCanvas(bool drawingFlag) {
	ofFbo & normalSource = (drawingFlag && !glowCanvas) ? preFbo2D : fbo2D;
	ofFbo & glowSource = (drawingFlag && glowCanvas) ? preFbo2D : glowCoreFbo;

#ifdef __EMSCRIPTEN__
#else
	const bool canDrawGlow = glowShader.isLoaded();

	if (canDrawGlow) {
		blurInto(glowSource, glowFbo1, 1.0f, 1.0f);
		blurInto(glowSource, glowFbo2, 4.0f, glowStrength);
	}
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

		glBlendFuncSeparate(
			GL_ONE, GL_ONE,
			GL_ZERO, GL_ONE
		);

		glowFbo2.draw(0, 0);

	   glBlendFuncSeparate(
			GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
			GL_ONE, GL_ONE_MINUS_SRC_ALPHA
	   );
	}
#endif // __EMSCRIPTEN__

	
}

void PaintTool::saveUndo() {
	undoFbos[undoWriteIndex].begin();

	ofClear(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	fbo2D.draw(0, 0);

	undoFbos[undoWriteIndex].end();

	undoWriteIndex = (undoWriteIndex + 1) % UNDO_CAPACITY;
	if (undoCount < UNDO_CAPACITY) {
		undoCount++;
	}
}

void PaintTool::undo() {
	if (undoCount <= 0) {
		return;
	}

	if (redoCount == 0) {
		redoFbo.begin();

		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		fbo2D.draw(0, 0);

		redoFbo.end();
	}

	undoWriteIndex--;

	if (undoWriteIndex < 0) {
		undoWriteIndex = UNDO_CAPACITY - 1;
	}

	fbo2D.begin();

	ofClear(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	undoFbos[undoWriteIndex].draw(0, 0);

	fbo2D.end();

	undoCount--;
	redoCount++;
}

void PaintTool::redo() {
	if (redoCount <= 0) {
		return;
	}

	undoWriteIndex++;

	if (undoWriteIndex == UNDO_CAPACITY) {
		undoWriteIndex = 0;
	}

	fbo2D.begin();
	ofClear(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	if (redoCount == 1) {
		redoFbo.draw(0, 0);
	} else {
		undoFbos[undoWriteIndex].draw(0, 0);
	}
	fbo2D.end();

	undoCount++;
	redoCount--;
}

void PaintTool::deleteAll() {
	saveUndo();
	fbo2D.begin();
	ofClear(0, 0, 0, 0);
	fbo2D.end();

	redoCount = 0;
}

#ifdef __EMSCRIPTEN__
#else
void PaintTool::setupGlow() {
	if (!glowShader.load("shaders/glowBlur")) {
		ofLogError("PaintTool") << "Failed to load glow shader";
	}
}

void PaintTool::blurInto(ofFbo& source, ofFbo& destination, float sigma, float strength) {
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
