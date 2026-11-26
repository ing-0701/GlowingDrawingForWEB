#pragma once

#include "ofMain.h"
#include "ofxGui.h"

#include "PaintLayer.h"
#include "LayerFilter.h"
#include "ColorPicker.h"
#include "BrushControls.h"
#include "LayerList.h"

#include <memory>
#include <vector>
#include <functional>

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
	paintcoreui::BrushControls brushControls;
	ofxToggle penSwitch;
	ofxButton undo;
	ofxButton redo;
	ofxButton deleteAll;
	ofColor pickedColor;

	Ui(int mainWidth);
	void updatePicker(int mainWidth);
	void picking(int x, int y, int mainWidth);

	void placeUi(bool isPen);
	void drawLayerPanel(
		const std::vector<std::unique_ptr<paintcore::Layer>> & layers,
		int selectedLayerId,
		const std::function<LayerType(int)>& getLayerType);

	LayerUiAction pressLayerPanel(
		int x,
		int y,
		const std::vector<std::unique_ptr<paintcore::Layer>> & layers);

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

	ofxButton backgroundButton;
	paintcoreui::ColorPicker backgroundPicker;

	bool backgroundPickerVisible = false;
	bool backgroundPicking = false;

	ofColor getBackgroundColor() const;
	bool isBackgroundPicking() const;
	bool beginBackgroundPicking(int x, int y);
	void dragBackgroundPicking(int x, int y);
	void endBackgroundPicking();
	void updateBackgroundPicker();

	void toggleBackgroundPicker();

	bool glowControlsEnabled = false;
#endif // __EMSCRIPTEN__


private:
	ofRectangle layerAddRect;

    bool layerMenuOpen = false;

	static constexpr int visibleLayerCount = 4;
	static constexpr float layerRowHeight = 46.0f;

	ofRectangle getLayerRowRect(int visibleRow) const;
	ofRectangle getLayerDeleteRect(int visibleRow) const;

	paintcoreui::ColorPicker colorPicker;
	paintcoreui::LayerList layerList;
};
