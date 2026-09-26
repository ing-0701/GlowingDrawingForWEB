#pragma once


#include "ofMain.h"

class PaintTool {
public:
	ofColor color;
	vector<glm::vec2> points;
	ofFbo fbo2D;
	ofFbo preFbo;

	ofPolyline line;

	PaintTool(int mainWidth, int mainHeight);

	void drawLine(bool penSwith, float eraserAlpha);
	void saveLine();
	void drawCanvas(int mainWidth, int mainHeight);
	void deleteAll();

	float strokeSize;
};
