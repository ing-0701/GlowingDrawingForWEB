#pragma once

#include "ofMain.h"
#include "ofxGui.h"

#include "Layer.h"

#include <memory>
#include <vector>

enum class LayerUiActionType {
	None,
	Select,
	AddNormal,
	AddGlow,
	Move,
	Delete
};

struct LayerUiAction {
	LayerUiActionType type = LayerUiActionType::None;
	int layerId = 0;
	int destinationIndex = -1;
};

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
	void drawLayerPanel(
		const std::vector<std::unique_ptr<Layer>> & layers,
		int selectedLayerId);

	LayerUiAction pressLayerPanel(
		int x,
		int y,
		const std::vector<std::unique_ptr<Layer>> & layers);

	void scrollLayerPanel(float scrollY, int layerCount);
	LayerUiAction releaseLayerPanel(
		int x,
		int y,
		int layerCount);

#ifdef __EMSCRIPTEN__
#else
	ofxFloatSlider glowSigma;
	ofxFloatSlider glowStrength;
	ofxFloatSlider glowPeriod;

	bool glowControlsEnabled = false;
#endif // __EMSCRIPTEN__


private:
	ofRectangle layerAddRect;
	ofRectangle layerListRect;

    bool layerMenuOpen = false;
	int firstVisibleLayer = 0;

	static constexpr int visibleLayerCount = 4;
	static constexpr float layerRowHeight = 46.0f;

	ofRectangle getLayerRowRect(int visibleRow) const;

	// マウスで押したレイヤー。0は押していない状態
	int draggedLayerId = 0;
	ofRectangle getLayerDeleteRect(int visibleRow) const;
};
