#include "ofApp.h"

bool modelingFlag = false;
bool drawingFlag = false;
bool pickerFlag = false;

//--------------------------------------------------------------
void ofApp::setup() {

	ofSetFrameRate(60);

	paintTool.fbo2D.begin();
	ofBackground(255);
	ofClear(255, 255, 255, 255);
	paintTool.fbo2D.end();

	//ofSetLineWidth(10);
	paintTool.color = ofColor(0, 0, 0, 255);
}

//--------------------------------------------------------------
void ofApp::update() {
}

//--------------------------------------------------------------
void ofApp::draw() {
	backgroundColor.setHsb(ui.hue * 255, 170, 170);
	ofBackground(backgroundColor);

	ui.placeUi(mainWidth);
	ui.updatePicker(mainWidth);
	if (pickerFlag) {
		ui.picking(ofGetMouseX(), ofGetMouseY(), mainWidth);
		paintTool.color = ofColor(ui.pickedColor);
	}

	paintTool.strokeSize = ui.penSize;
	paintTool.color = ofColor(paintTool.color.r, paintTool.color.g, paintTool.color.b, ui.penAlpha * 255);

	paintTool.drawLine(ui.penSwitch, ui.eraserAlpha * 255);
	paintTool.drawCanvas(mainWidth, mainHeight);
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

			vector<glm::vec2> newPoints; //Points for interpolation
			float pointsDist = glm::distance(paintTool.points[paintTool.points.size() - 1], paintTool.points[paintTool.points.size() - 2]);

			float idealDist = 3;
			if (pointsDist > idealDist) {
				float NumberOfPoint = pointsDist / idealDist;
				float span = 1 / NumberOfPoint;

				for (float i = span; i < 1; i += span) {
					glm::vec2 point = glm::mix(paintTool.points[paintTool.points.size() - 1], paintTool.points[paintTool.points.size() - 2], i);

					float lastDist = glm::distance(point, paintTool.points[paintTool.points.size() - 2]);
					//`lastDist` is the distance between the last point to be interpolated and the last point before interpolation.
					//If `lastDist` is too small, the last point is unnecessary.
					if (lastDist > 0.7) {
						newPoints.push_back(point);
					}
				}

				for (int i = 0; i < newPoints.size(); i++) {
					paintTool.points.insert(paintTool.points.begin() + paintTool.points.size() - 1, newPoints[i]);
				}
				newPoints.clear();
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
	if (0 <= x && x < mainWidth && 0 <= y && y < mainHeight) {
		drawingFlag = true;
	}

	if (mainWidth <= x && x < ofGetWidth() && 0 <= y && y < 256) {
		pickerFlag = true;
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	paintTool.fbo2D.begin();
	ofSetColor(255, 255, 255, 255);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	paintTool.preFbo.draw(0, 0, mainWidth, mainHeight);
	paintTool.fbo2D.end();
	paintTool.points.clear();

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
