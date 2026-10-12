#pragma once


#include "ofMain.h"

class PaintTool {
public:
	ofColor color;
	vector<glm::vec2> points;
	ofFbo fbo2D;
	ofFbo preFbo2D;
	ofFbo redoFbo;
	ofFbo glowFbo;
	static const int UNDO_CAPACITY = 20;
	ofFbo undoFbos[UNDO_CAPACITY];
	int undoWriteIndex = 0;
	int undoCount = 0;
	int redoCount = 0;

	PaintTool(int mainWidth, int mainHeight);

	void drawLine(bool penSwith, float eraserAlpha);
	void drawCanvas(bool drawingFlag);
	void saveUndo();
	void undo();
	void redo();
	void deleteAll();

	float strokeSize;

#ifdef __EMSCRIPTEN__
#else
	ofFbo preGlowFbo;
	ofShader glowShader;

	float glowStrength = 1.0f;

	void setupGlow();
	void updateGlow(ofFbo& source);
#endif // __EMSCRIPTEN__
};
