#include "ofApp.h"

#ifdef __EMSCRIPTEN__ //以下#endifまで、emscriptenでビルドするときのみ有効
	#include <emscripten.h>

EM_JS(int, getBackgroundBrightness, (), {
	return window['paintBackgroundBrightness'] ?? 255;
})

EM_JS(double, getPenSize, (), {
	return window['paintPenSize'] ?? 0.1;
})

EM_JS(double, getPenAlpha, (), {
	return window['paintPenAlpha'] ?? 1;
})

EM_JS(double, getEraserSize, (), {
	return window['paintEraserSize'] ?? 0.1;
})

EM_JS(double, getEraserAlpha, (), {
	return window['paintEraserAlpha'] ?? 1;
})

EM_JS(int, getPenMode, (), {
	const value = window['paintPenMode'];

	if (typeof value === 'boolean') {
		return value ? 1 : 0;
	}

	return 1;
})

EM_JS(int, takePaintCommand, (), {
	const commands = window['paintCommands'];

	if (!commands || commands.length === 0) {
		return 0;
	}

	const command = commands.shift();

	if (command === 'undo') return 1;
	if (command === 'redo') return 2;
	if (command === 'deleteAll') return 3;

	return 0;
})
#endif

bool modelingFlag = false;
bool drawingFlag = false;
bool pickerFlag = false;

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetFrameRate(60);

	paintTool.fbo2D.begin();
	ofClear(0, 0, 0, 0);
	paintTool.fbo2D.end();

	ui.undo.addListener(&paintTool, &PaintTool::undo);
	ui.redo.addListener(&paintTool, &PaintTool::redo);
	ui.deleteAll.addListener(&paintTool, &PaintTool::deleteAll);

#ifdef __EMSCRIPTEN__
#else
	paintTool.setupGlow();
#endif // __EMSCRIPTEN__
}

//--------------------------------------------------------------
void ofApp::update() {
#ifdef __EMSCRIPTEN__
	if (drawingFlag) {
		return;
	}

	int command = takePaintCommand();

	while (command != 0) {
		if (command == 1) {
			paintTool.undo();
		} else if (command == 2) {
			paintTool.redo();
		} else if (command == 3) {
			paintTool.deleteAll();
		}

		command = takePaintCommand();
	}
#endif // __EMSCRIPTEN__
}

//--------------------------------------------------------------
void ofApp::draw() {
#ifdef __EMSCRIPTEN__
	ofBackground(getBackgroundBrightness());
	const bool isPen = getPenMode() != 0;

	if (isPen) {
		paintTool.strokeSize = getPenSize();
	} else {
		paintTool.strokeSize = getEraserSize();
	}
	paintTool.color = ofColor(paintTool.color.r, paintTool.color.g, paintTool.color.b, getPenAlpha() * 255);
	if (drawingFlag) {
		paintTool.drawLine(isPen, getEraserAlpha() * 255);
	}
#else
	ofBackground(20);
	const bool isPen = ui.penSwitch;

	if (isPen) {
		paintTool.strokeSize = ui.penSize;
	} else {
		paintTool.strokeSize = ui.eraserSize;
	}
	paintTool.color = ofColor(paintTool.color.r, paintTool.color.g, paintTool.color.b, ui.penAlpha * 255);
	if (drawingFlag) {
		paintTool.drawLine(isPen, ui.eraserAlpha * 255);
	}
#endif // __EMSCRIPTEN__
	paintTool.drawCanvas(drawingFlag);

	ofSetColor(100);
	ofDrawRectangle(800, 256, 256, 344);

	ui.placeUi(isPen);
	ui.updatePicker(mainWidth);
	if (pickerFlag) {
		ui.picking(ofGetMouseX(), ofGetMouseY(), mainWidth);
		paintTool.color = ui.pickedColor;
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
	if (drawingFlag) {
		paintTool.points.push_back(glm::vec2(x, y));

		//"When the coordinates are too far apart, interpolate the values in between"
		if (paintTool.points.size() > 1) {
			const glm::vec2 currentPoint = paintTool.points.back();
			const glm::vec2 previousPoint = paintTool.points[paintTool.points.size() - 2];

			vector<glm::vec2> newPoints; //Points for interpolation
			const float pointsDist = glm::distance(currentPoint, previousPoint);
			float idealDist = 3;
			if (pointsDist > idealDist) {
				float NumberOfPoint = pointsDist / idealDist;
				float span = 1 / NumberOfPoint;

				for (float i = span; i < 1; i += span) {
					glm::vec2 point = glm::mix(currentPoint, previousPoint, i);
					float lastDist = glm::distance(point, previousPoint); //`lastDist` is the distance between the last point to be interpolated and the last point before interpolation.
					//If `lastDist` is too small, the last point is unnecessary.
					if (lastDist > 0.7) {
						newPoints.push_back(point);
					}
				}

				paintTool.points.insert(
					paintTool.points.end() - 1,
					newPoints.begin(),
					newPoints.end());
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
	if (button != OF_MOUSE_BUTTON_LEFT) {
		return;
	}

	if (0 <= x && x < mainWidth && 0 <= y && y < mainHeight) {
#ifdef __EMSCRIPTEN__
		paintTool.glowCanvas = false;
#else
		paintTool.glowCanvas = ui.glowSwitch;
#endif // __EMSCRIPTEN__

		drawingFlag = true;
		paintTool.points.push_back(glm::vec2(x, y));
	}

	if (mainWidth <= x && x < ofGetWidth() && 0 <= y && y < 256) {
		pickerFlag = true;
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	if (button != OF_MOUSE_BUTTON_LEFT) {
		return;
	}

	if (drawingFlag) {
#ifdef __EMSCRIPTEN__
		const bool isPen = getPenMode() != 0;
		if (isPen) {
			paintTool.strokeSize = getPenSize();
		} else {
			paintTool.strokeSize = getEraserSize();
		}
		paintTool.color = ofColor(paintTool.color.r, paintTool.color.g, paintTool.color.b, getPenAlpha() * 255);
		paintTool.drawLine(isPen, getEraserAlpha() * 255);
#else
		const bool isPen = ui.penSwitch;
		if (isPen) {
			paintTool.strokeSize = ui.penSize;
		} else {
			paintTool.strokeSize = ui.eraserSize;
		}
		paintTool.color = ofColor(paintTool.color.r, paintTool.color.g, paintTool.color.b, ui.penAlpha * 255);
		paintTool.drawLine(isPen, ui.eraserAlpha * 255);
#endif

		ofFbo & destination = paintTool.glowCanvas ? paintTool.glowCoreFbo : paintTool.fbo2D;

		paintTool.saveUndo();
		destination.begin();
		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		paintTool.preFbo2D.draw(0, 0, mainWidth, mainHeight);
		destination.end();
		paintTool.points.clear();

		paintTool.redoCount = 0;
	}

	drawingFlag = false;
	pickerFlag = false;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
}

//--------------------------------------------------------------
void ofApp::exit() {
	ui.undo.removeListener(&paintTool, &PaintTool::undo);
	ui.redo.removeListener(&paintTool, &PaintTool::redo);
	ui.deleteAll.removeListener(&paintTool, &PaintTool::deleteAll);
}
