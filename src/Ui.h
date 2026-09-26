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
	ofFbo pickerFbo;
	ofPixels pickerPix;
	ofTexture pickerTex;
	ofColor pickedColor;

	Ui();
	void updatePicker(int mainWidth);
	void picking(int x, int y, int mainWidth);

	void placeUi(int mainWidth, bool isPen);
	void buttonPressed();
};
