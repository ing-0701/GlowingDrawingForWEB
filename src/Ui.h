#pragma once

#include "ofMain.h"
#include "ofxGui.h"

class Ui {
public:
	ofxFloatSlider hue;
	ofxFloatSlider penSize;
	ofxFloatSlider penAlpha;
	ofxFloatSlider eraserSize;
	ofxFloatSlider eraserAlpha;
	ofxToggle penSwitch;
	ofxButton undo;
	ofxButton redo;
	ofxButton deleteAll;
	ofPixels pickerPix;
	ofTexture pickerTex;
	ofColor pickedColor;

	Ui(int mainWidth);
	void updatePicker(int mainWidth);
	void picking(int x, int y, int mainWidth);

	void placeUi(bool isPen);
	float lastHue = -1.0f;
};
