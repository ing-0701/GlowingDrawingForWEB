#pragma once

#include "PaintTool.h"
#include "Ui.h"
#include "ofMain.h"

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();
	void exit() override;

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

	void beforeDeleteAll();
    paintcore::BrushSettings readBrushSettings();

	const static int mainWidth = 800;
	const static int mainHeight = 600;

	PaintTool paintTool = PaintTool(mainWidth, mainHeight);
	ofPixels pixels;
    ofColor brushColor = ofColor::black;

	Ui ui = Ui(mainWidth);

	void mouseScrolled(
		int x,
		int y,
		float scrollX,
		float scrollY) override;

#ifdef __EMSCRIPTEN__
	void syncLayerStateToWeb();
#else

	// Glowスライダーの表示処理
	void syncGlowControls();

	// スライダーの更新を受けてパラメータ更新
	void onGlowParameterChanged(float & value);

	// スライダーの値をレイヤーごとの変数に同期している最中かの判定
	bool syncingGlowControls = false;

	// Glow関連スライダーの対象となってるレイヤー
	int glowControlsLayerId = 0;

#endif // __EMSCRIPTEN__
};
