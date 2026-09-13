#pragma once

#include "Ui.h"

Ui::Ui() {
	ofxGuiSetDefaultWidth(256);
	ofxGuiSetDefaultHeight(10);
	hue.setup("Hue", 0.0, 0.0, 1.0);
	hue.setSize(256, 30);
	penSize.setup("penSize", 0.1, 0.1, 1.0);
	penSize.setSize(256, 30);
	penAlpha.setup("penAlpha", 1.0, 0.0, 1.0);
	penAlpha.setSize(256, 30);
	eraserSize.setup("eraserSize", 0.1, 0.1, 1.0);
	eraserSize.setSize(256, 30);
	eraserAlpha.setup("eraserAlpha", 1.0, 0.0, 1.0);
	eraserAlpha.setSize(256, 30);
	image3D.setup("3Dimage", 0.0, 0.0, 1.0);
	image3D.setSize(256, 30);
	penSwitch.setup("pen/eraser", true);
	penSwitch.setSize(100, 30);

	ofFbo::Settings s;
	s.width = 256;
	s.height = 256;
	s.internalformat = GL_RGBA32F;

	pickerFbo.allocate(s);
	pickerPix.allocate(256, 256, OF_PIXELS_RGBA);
	pickerTex.allocate(pickerPix);

	pickerFbo.begin();
	ofClear(255, 255, 255, 255);
	pickerFbo.end();
}

void Ui::updatePicker(int mainWidth) {
	for (int y = 0; y < 256; y++) {
		for (int x = 0; x < 256; x++) {
			ofColor pixColor;
			pixColor.setHsb(hue * 255, x, 255 - y);
			pickerPix.setColor(x, y, pixColor);
		}
	}
	pickerTex.loadData(pickerPix);

	ofSetColor(255, 255, 255, 255);

	pickerFbo.begin();
	pickerTex.draw(0, 0);
	pickerFbo.end();

	pickerFbo.draw(mainWidth, 0, 256, 256);
}

void Ui::picking(int x, int y, int mainWidth) {

	if (x < mainWidth) {
		x = mainWidth;
	} else if (ofGetWidth() <= x) {
		x = ofGetWidth() - 1;
	}

	if (y < 0) {
		y = 0;
	} else if (255 < y) {
		y = 255;
	}

	ofColor nowColor;
	nowColor.setHsb(hue * 255, x - mainWidth, 255 - y);

	ofFill();
	ofSetColor(nowColor);
	ofDrawCircle(x, y, 10);

	ofNoFill();
	ofSetColor(150, 150, 150);
	ofSetLineWidth(2);
	ofDrawCircle(x, y, 10);

	pickedColor = ofColor(nowColor);
}

void Ui::placeUi(int mainWidth) {
	hue.setPosition(mainWidth, 258 + 32 * 0);
	penSize.setPosition(mainWidth, 258 + 32 * 1);
	penAlpha.setPosition(mainWidth, 258 + 32 * 2);
	eraserSize.setPosition(mainWidth, 258 + 32 * 1);
	eraserAlpha.setPosition(mainWidth, 258 + 32 * 2);
	image3D.setPosition(mainWidth, 258 + 32 * 3);
	penSwitch.setPosition(mainWidth, 258 + 32 * 4);

	if (penSwitch) {
		hue.draw();
		penSize.draw();
		penAlpha.draw();
		image3D.draw();
		penSwitch.draw();
	} else {
		hue.draw();
		eraserSize.draw();
		eraserAlpha.draw();
		image3D.draw();
		penSwitch.draw();
	}
	
}
