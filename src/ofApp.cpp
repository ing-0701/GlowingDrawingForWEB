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

	ofFbo::Settings s;
	s.width = mainWidth;
	s.height = mainHeight / 2;
	s.internalformat = GL_RGBA;
	s.useDepth = true;
	s.depthStencilAsTexture = true;
	s.depthStencilInternalFormat = GL_DEPTH_COMPONENT24;
	fbo3D.allocate(s);

	fbo3D.begin();
	light1.setSpotlight(70, 5); //(angle:0 > > >180, conentration: 0 > > > 128)
	light1.setPosition(-100, 200, 600);

	light2.setSpotlight(70, 5); //(angle:0 > > >180, conentration: 0 > > > 128)
	light2.setPosition(400, -300, -1600);
	light2.setDiffuseColor(ofFloatColor(0.6, 0.6, 0.6));
	light2.lookAt(glm::vec3(0, 0, 0));
	fbo3D.end();

	//ofSetLineWidth(10);
	paintTool.color = ofColor(0, 0, 0, 255);

	ofIcoSpherePrimitive sphere;
	sphere.setRadius(100);
	sphere.setResolution(5);
	mesh = sphere.getMesh();

	plane.set(1000, 500, 1000, 500);
	mesh2D = plane.getMesh();
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

	if (modelingFlag) {
		ofTexture & depthTex = fbo3D.getDepthTexture();

		vector<float> depth(depthTex.getWidth() * depthTex.getHeight());
		GLenum target = depthTex.getTextureData().textureTarget;
		glBindTexture(target, depthTex.getTextureData().textureID);
		//glGetTexImage(target, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());
		glBindTexture(target, 0);

		cout << depthTex.getTextureData().textureTarget << endl;
		cout << depthTex.getTextureData().textureID << endl;
		GLenum err = glGetError();
		cout << err << endl;
		cout << depthTex.isAllocated() << endl;

		fbo3D.begin();
		ofEnableDepthTest();
		ofClear(0, 0, 0, 255);
		cam.begin();

		//Plane
		//for (int i = 0; i < mesh2D.getVertices().size(); i++) {
		//	glm::vec3 loc = mesh2D.getVertices()[i];

		//	glm::vec2 loc2D = cam.worldToScreen(loc);

		//	int ix = round(loc2D.x); // float -> int
		//	int iy = round(loc2D.y); // float -> int

		//	if (iy * mainWidth + ix < brightness.size() && 0 <= ix && ix < mainWidth && 0 <= iy && iy < mainHeight / 2) {
		//		float brightness3D = brightness[iy * mainWidth + ix];
		//		mesh2D.setVertex(i, loc + glm::vec3(0, 0, -255 + brightness3D));
		//	}
		//}


		//sphere
		for (int i = 0; i < mesh.getVertices().size(); i++) {
			glm::vec3 loc = mesh.getVertices()[i];

			glm::vec3 loc2D = cam.worldToScreen(loc);

			glm::vec3 locN = glm::normalize(loc);
			float locL = glm::length(loc);


			int ix = (int)loc2D.x; // float -> int
			int iy = (int)loc2D.y; // float -> int

			if (iy * mainWidth + ix < brightness.size() && 0 <= ix && ix < mainWidth && 0 <= iy && iy < mainHeight / 2) {
				if (loc2D.z <= depth[ix + iy * mainWidth] - 0.005f) {
					float brightness3D = brightness[iy * mainWidth + ix];
					mesh.setVertex(i, locN * locL * (brightness3D * (0.3 / 255) + 0.7));
				}
			}
		}

		modelingFlag = false;

		cam.end();
		ofDisableDepthTest();
		fbo3D.end();
	}

	fbo3D.begin();

	ofClear(100, 100, 100, 255);

	ofEnableDepthTest();

	ofEnableLighting();
	light1.enable();
	light2.enable();

	cam.begin();

	ofSetColor(255);

	mesh.draw();
	//mesh2D.draw();

	cam.end();

	light1.disable();
	light2.disable();
	ofDisableLighting();

	fbo3D.end();

	ofSetColor(255, 255, 255, 255);
	fbo3D.draw(0, 0, mainWidth, mainHeight / 2);

	ofSetColor(255, 255, 255, ui.image3D * 255);
	fbo3D.draw(0, mainHeight / 2, mainWidth, mainHeight / 2);

	ofDisableDepthTest();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

	if (key == 'e') {
		paintTool.color = ofColor(255, 255, 255, 100);
	} else if (key == 'g') {

		pixels.clear();
		brightness.clear();
		paintTool.fbo2D.readToPixels(pixels);

		for (int y = 0; y < mainHeight / 2; y++) {
			for (int x = 0; x < mainWidth; x++) {
				ofColor c = pixels.getColor(x, y);
				brightness.push_back(c.getBrightness());
			}
		}
		ofSaveImage(pixels, "test_0713_1.png");
		modelingFlag = true;

		cout << brightness.size() << "--------------------brightness. \n";
	} else if (key == 't') {
		ofSetColor(paintTool.color);
		paintTool.preFbo.begin();
		ofClear(255, 255, 255, 0);
		glm::vec2 r = glm::vec2(ofRandom(-50, 50), ofRandom(-50, 50));
		for (int i = 0; i < 2; i++) {
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			ofDrawCircle(mainWidth / 2 + r.x, mainHeight / 4 + r.y, 100);
		}
		paintTool.preFbo.end();
		cout << paintTool.color << endl;

		paintTool.fbo2D.begin();
		ofSetColor(255, 255, 255, 255);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
		paintTool.preFbo.draw(0, 0, mainWidth, mainHeight / 2);
		paintTool.fbo2D.end();
	} else if (key == 'y') {
		ofPixels testP2;
		testP2.allocate(mainWidth, mainHeight / 2, OF_PIXELS_RGBA);

		for (int y = 0; y < mainHeight / 2; y++) {
			for (int x = 0; x < mainWidth; x++) {

				float value = (float)x / mainWidth * 20.0f
					* (float)y / (mainHeight / 2) * 20.0f;

				if (value > 255) {
					value = 255;
				}

				testP2.setColor(
					x,
					y,
					ofColor(value, value, value, 255));
			}
		}

		ofTexture testT2;
		testT2.allocate(testP2);
		testT2.loadData(testP2);

		paintTool.fbo2D.begin();
		testT2.draw(0, 0);
		paintTool.fbo2D.end();
	} else if (key == 'c') {
		fbo3D.begin();
		cam.begin();

		cout << brightness.size() << "--------------------brightness. \n";
		cout << cam.worldToScreen(mesh2D.getVertices()[0]) << "--------------------getVertices()[0] \n";
		cout << cam.worldToScreen(mesh2D.getVertices()[1023]) << "--------------------getVertices()[1023] \n";
		cout << cam.worldToScreen(mesh2D.getVertices()[mesh2D.getVertices().size() - 1024]) << "--------------------getVertices()[mesh2D.getVertices().size()-1024] \n";
		cout << cam.worldToScreen(mesh2D.getVertices()[mesh2D.getVertices().size() - 1]) << "--------------------getVertices()[mesh2D.getVertices().size()-1] \n";

		cout << "window: "
			 << mainWidth
			 << " "
			 << mainHeight
			 << endl;

		cout << "window pixel: "
			 << ofGetViewportWidth()
			 << " "
			 << ofGetViewportHeight()
			 << endl;

		ofRectangle viewport = ofGetCurrentViewport();

		cout << viewport.x << endl;
		cout << viewport.y << endl;
		cout << viewport.width << endl;
		cout << viewport.height << endl;

		cout << "cam position : "
			 << cam.getPosition()
			 << endl;

		cout << "cam fov : "
			 << cam.getFov()
			 << endl;

		cout << "aspect : "
			 << cam.getAspectRatio()
			 << endl;

		cout << cam.getModelViewProjectionMatrix()
			 << endl;

		cout << cam.worldToScreen(glm::vec3(0, 0, 0))
			 << endl;

		vector<glm::vec3> testPoints;

		testPoints.push_back(glm::vec3(-1000, 0, 0));
		testPoints.push_back(glm::vec3(0, 0, 0));
		testPoints.push_back(glm::vec3(1000, 0, 0));

		cout << ofGetCurrentViewport() << endl;

		for (auto p : testPoints) {
			cout
				<< p
				<< " -> "
				<< cam.worldToScreen(p)
				<< endl;
		}

		cam.end();
		fbo3D.end();
	}
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
		paintTool.points.push_back(glm::vec2(x, y - mainHeight / 2));

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
	if (0 <= x && x < mainWidth && mainHeight / 2 <= y && y < mainHeight) {
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
	paintTool.preFbo.draw(0, 0, mainWidth, mainHeight / 2);
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
