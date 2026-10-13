#pragma once


#include "ofMain.h"

class PaintTool {
public:
	ofColor color;
	vector<glm::vec2> points;
	ofFbo fbo2D;
	ofFbo preFbo2D;
	ofFbo historySwapFbo;
	ofFbo glowCoreFbo;
	ofFbo preGlowFbo;
	ofFbo glowFbo1;
	ofFbo glowFbo2;
	static const int UNDO_CAPACITY = 20;
	ofFbo undoFbos[UNDO_CAPACITY];
	int undoTargets[UNDO_CAPACITY] = {};
	int undoWriteIndex = 0;
	int undoCount = 0;
	int redoCount = 0;

	PaintTool(int mainWidth, int mainHeight);

	void drawLine(bool penSwith, float eraserAlpha);
	void drawCanvas(bool drawingFlag);
	void copyFbo(ofFbo & source, ofFbo & destination);
	void swapHistory(int index);
	void saveUndo();
	void undo();
	void redo();
	void deleteAll();

	float strokeSize;
	bool glowCanvas = false;
	bool glowNeedUpdate = true;

#ifdef __EMSCRIPTEN__
#else
	ofShader glowShader;

	float glowStrength = 2.0f;

	void setupGlow();
	void blurInto(ofFbo& source, ofFbo& destination, float sigma, float strength);
#endif // __EMSCRIPTEN__
};
