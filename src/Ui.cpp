#include "Ui.h"

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
EM_JS(double, getHue, (), {
	return window['paintHue'] ?? 0;
})
#endif

Ui::Ui(int mainWidth) {
	ofxGuiSetDefaultWidth(256);
	ofxGuiSetDefaultHeight(10);

#ifndef __EMSCRIPTEN__
	hue.setup("Hue", 0.0, 0.0, 1.0);
	hue.setSize(256, 30);
	hue.setPosition(mainWidth, 258 + 32 * 0);
	penSize.setup("penSize", 0.1, 0.1, 1.0);
	penSize.setSize(256, 30);
	penSize.setPosition(mainWidth, 258 + 32 * 1);
	penSwitch.setup("pen/eraser", true);
	penSwitch.setSize(100, 30);
	penSwitch.setPosition(mainWidth, 258 + 32 * 4);
	glowSwitch.setup("normal/glow", false);
	glowSwitch.setSize(100, 30);
	glowSwitch.setPosition(mainWidth+100, 258 + 32 * 4);
	penAlpha.setup("penAlpha", 1.0, 0.0, 1.0);
	penAlpha.setSize(256, 30);
	penAlpha.setPosition(mainWidth, 258 + 32 * 2);
	eraserSize.setup("eraserSize", 0.1, 0.1, 1.0);
	eraserSize.setSize(256, 30);
	eraserSize.setPosition(mainWidth, 258 + 32 * 1);
	eraserAlpha.setup("eraserAlpha", 1.0, 0.0, 1.0);
	eraserAlpha.setSize(256, 30);
	eraserAlpha.setPosition(mainWidth, 258 + 32 * 2);
	undo.setup("undo");
	undo.setSize(100, 30);
	undo.setPosition(mainWidth, 258 + 32 * 5);
	redo.setup("redo");
	redo.setSize(100, 30);
	redo.setPosition(mainWidth, 258 + 32 * 6);
	deleteAll.setup("delete");
	deleteAll.setSize(100, 30);
	deleteAll.setPosition(mainWidth, 258 + 32 * 7);
#endif // !__EMSCRIPTEN__

	pickerPix.allocate(256, 256, OF_PIXELS_RGBA);
	pickerTex.allocate(pickerPix);
}

void Ui::updatePicker(int mainWidth) {
#ifdef __EMSCRIPTEN__
	const float currentHue = static_cast<float>(getHue());
#else
	const float currentHue = hue;
#endif

	if (currentHue != lastHue) {
		for (int y = 0; y < 256; y++) {
			for (int x = 0; x < 256; x++) {
				ofColor pixColor;
				pixColor.setHsb(currentHue * 255, x, 255 - y);

				pickerPix.setColor(x, y, pixColor);
			}
		}
		pickerTex.loadData(pickerPix);
		lastHue = currentHue;
	}

	ofSetColor(255, 255, 255, 255);
	pickerTex.draw(mainWidth, 0, 256, 256);
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
#ifdef __EMSCRIPTEN__
	nowColor.setHsb(getHue() * 255, x - mainWidth, 255 - y);
#else
	nowColor.setHsb(hue * 255, x - mainWidth, 255 - y);
#endif // __EMSCRIPTEN__

	ofFill();
	ofSetColor(nowColor);
	ofDrawCircle(x, y, 10);

	ofNoFill();
	ofSetColor(150, 150, 150);
	ofSetLineWidth(2);
	ofDrawCircle(x, y, 10);

	pickedColor = nowColor;
}

void Ui::placeUi(bool isPen) {

#ifndef __EMSCRIPTEN__
	hue.draw();
	if (isPen) {
		penSize.draw();
		penAlpha.draw();
	} else {
		eraserSize.draw();
		eraserAlpha.draw();
	}
	penSwitch.draw();
	glowSwitch.draw();
	undo.draw();
	redo.draw();
	deleteAll.draw();
#endif // __EMSCRIPTEN__
}
