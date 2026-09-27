#include "PaintTool.h"

PaintTool::PaintTool(int mainWidth, int mainHeight) {
	color = (0, 0, 0, 255);
	strokeSize = 0;

	ofFbo::Settings s;
	s.width = mainWidth;
	s.height = mainHeight;
	s.internalformat = GL_RGBA32F;

	fbo2D.allocate(s);
	preFbo.allocate(s);
	redoFbo.allocate(s);
	for (int i = 0; i < UNDO_CAPACITY; i++) {
		undoFbos[i].allocate(s);
	}
}

void PaintTool::drawLine(bool penSwitch, float eraserAlpha) {
	ofSetLineWidth(strokeSize * 50);
	ofFill();
	if (penSwitch) {
		ofSetColor(color);
	} else {
		ofSetColor(255, 255, 255, eraserAlpha);
	}

	line.clear();

	preFbo.begin();
	ofClear(0, 0, 0, 0);

	if (points.size() > 0) {
		//パスで線を書いていた時の残骸
		//ofSetColor(color);
		//for (int i = 0; i < points.size(); i++) {
		//	line.addVertex(points[i].x, points[i].y);
		//}
		//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

		//ofColor c = ofGetStyle().color;

		//line.draw();

		//ofDrawLineで線を書いていた時の残骸
		for (int i = 0; i < points.size(); i++) {
			if (i == 0) {
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				ofDrawCircle(points[i].x, points[i].y, strokeSize * 25);
			} else {
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				//ofDrawLine(points[i - 1].x, points[i - 1].y, points[i].x, points[i].y);
				ofDrawCircle(points[i].x, points[i].y, strokeSize * 25);
			}
		}
	}
	preFbo.end();
}

void PaintTool::drawCanvas(int mainWidth, int mainHeight) {
	ofSetColor(255, 255, 255, 255);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	fbo2D.draw(0, 0, mainWidth, mainHeight);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	preFbo.draw(0, 0, mainWidth, mainHeight);
}

void PaintTool::saveUndo() {
	undoFbos[undoWriteIndex].begin();

	ofClear(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	fbo2D.draw(0, 0);

	undoFbos[undoWriteIndex].end();

	undoWriteIndex = (undoWriteIndex + 1) % UNDO_CAPACITY;
	if (undoCount < UNDO_CAPACITY) {
		undoCount++;
	}
}

void PaintTool::undo() {
	if (undoCount <= 0) {
		return;
	}

	if (redoCount == 0) {
		redoFbo.begin();

		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		fbo2D.draw(0, 0);

		redoFbo.end();
	}

	undoWriteIndex--;

	if (undoWriteIndex < 0) {
		undoWriteIndex = UNDO_CAPACITY - 1;
	}

	fbo2D.begin();

	ofClear(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	undoFbos[undoWriteIndex].draw(0, 0);

	fbo2D.end();

	undoCount--;
	redoCount++;
}

void PaintTool::redo() {
	if (redoCount <= 0) {
		return;
	}

	undoWriteIndex++;

	if (undoWriteIndex == UNDO_CAPACITY) {
		undoWriteIndex = 0;
	}

	if (redoCount == 1) {
		fbo2D.begin();

		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		redoFbo.draw(0, 0);

		fbo2D.end();
	} else {
		fbo2D.begin();

		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		undoFbos[undoWriteIndex].draw(0, 0);

		fbo2D.end();
	}

	undoCount++;
	redoCount--;
}

void PaintTool::deleteAll() {
	saveUndo();
	fbo2D.begin();
	ofClear(0, 0, 0, 0);
	fbo2D.end();

	redoCount = 0;
}
