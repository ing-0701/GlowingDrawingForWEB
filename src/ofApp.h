#pragma once

#include "PaintTool.h"
#include "Ui.h"
#include "ofMain.h"

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	const static int mainWidth = 1000;
	const static int mainHeight = 1000;

	PaintTool paintTool = PaintTool(mainWidth, mainHeight);
	ofPixels pixels;
	vector<float> brightness;

	ofFbo fbo3D;

	ofEasyCam cam;
	ofMesh mesh;
	ofMesh mesh2D;
	ofPlanePrimitive plane;

	ofLight light1;
	ofLight light2;

	Ui ui;
	ofColor backgroundColor;
};
