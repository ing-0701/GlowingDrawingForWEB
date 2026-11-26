#include "Ui.h"
#include <algorithm>

Ui::Ui(int mainWidth) {
	ofxGuiSetDefaultWidth(256);
	ofxGuiSetDefaultHeight(10);

#ifndef __EMSCRIPTEN__
	const float actionX = static_cast<float>(mainWidth);
	const float actionWidth = 100.0f;
	const float controlsX = static_cast<float>(mainWidth + 104);
	const float controlsWidth = 148.0f;

	colorPicker.setup(mainWidth, 0);

	brushControls.setup(
		controlsX,
		290.0f,
		controlsWidth);
	penSwitch.setup("pen/eraser", true);
	penSwitch.setSize(100, 30);
	penSwitch.setPosition(actionX, 290.0f);
	undo.setup("undo");
	undo.setSize(100, 30);
	undo.setPosition(actionX, 322.0f);
	redo.setup("redo");
	redo.setSize(100, 30);
	redo.setPosition(actionX, 354.0f);
	deleteAll.setup("delete");
	deleteAll.setSize(100, 30);
	deleteAll.setPosition(actionX, 386.0f);
	glowSigma.setup("sigma", 2.5f, 1.0f, 4.0f);
	glowSigma.setSize(256, 30);
	glowSigma.setPosition(mainWidth, 606);
	glowStrength.setup("glowStrength", 4.5f, 1.5f, 7.5f);
	glowStrength.setSize(256, 30);
	glowStrength.setPosition(mainWidth, 638);
	glowPeriod.setup("period", 1.0f, 0.3f, 3.0f);
	glowPeriod.setSize(256, 30);
	glowPeriod.setPosition(mainWidth, 670);
	backgroundButton.setup("background");
	backgroundButton.setSize(actionWidth, 30.0f);
	backgroundButton.setPosition(actionX, 418.0f);
	backgroundButton.addListener(this, &Ui::toggleBackgroundPicker);

	backgroundPicker.setup(actionX, 452.0f, actionWidth);
	backgroundPicker.setColor(ofColor(20));
	backgroundPicker.setEnabled(false);

	// 起動時はNormalレイヤーなので、操作を受け付けない
	glowSigma.unregisterMouseEvents();
	glowStrength.unregisterMouseEvents();
	glowPeriod.unregisterMouseEvents();
#endif // !__EMSCRIPTEN__

	layerAddRect.set(
		mainWidth + 104, 354,
		148, 28);

	layerList.setup(
		mainWidth + 104, 386,
		148, layerRowHeight, visibleLayerCount);
}

#ifndef __EMSCRIPTEN__
void Ui::toggleBackgroundPicker() {
	backgroundPickerVisible = !backgroundPickerVisible;
	backgroundPicker.setEnabled(backgroundPickerVisible);
}

ofColor Ui::getBackgroundColor() const {
	return backgroundPicker.getColor();
}

bool Ui::isBackgroundPicking() const {
	return backgroundPicking;
}

bool Ui::beginBackgroundPicking(int x, int y) {
	if (!backgroundPickerVisible || !backgroundPicker.contains(x, y)) {
		return false;
	}

	backgroundPicking = true;
	backgroundPicker.select(x, y);
	return true;
}

void Ui::dragBackgroundPicking(int x, int y) {
	if (backgroundPicking) {
		backgroundPicker.select(x, y);
	}
}

void Ui::endBackgroundPicking() {
	backgroundPicking = false;
}

void Ui::updateBackgroundPicker() {
	backgroundPicker.update();
}

#endif // !__EMSCRIPTEN__

void Ui::updatePicker(int mainWidth) {
#ifdef __EMSCRIPTEN__
#else
	colorPicker.drawPicker();
#endif
}

void Ui::picking(int x, int y, int mainWidth) {
#ifdef __EMSCRIPTEN__
#else
	colorPicker.pick(x, y);
	pickedColor = colorPicker.getColor();
#endif // __EMSCRIPTEN__
}

void Ui::placeUi(bool isPen) {

#ifndef __EMSCRIPTEN__
	colorPicker.drawHueSlider();
	brushControls.draw(isPen);
	penSwitch.draw();
	undo.draw();
	redo.draw();
	deleteAll.draw();
	backgroundButton.draw();

	if (backgroundPickerVisible) {
		backgroundPicker.drawPicker();
		backgroundPicker.drawSelection();
		backgroundPicker.drawHueSlider();
	}

	if (glowControlsEnabled) {
		glowSigma.draw();
		glowStrength.draw();
		glowPeriod.draw();
	}
#endif // __EMSCRIPTEN__
}

// レイヤーの矩形を得る関数
// 引数は「上から何個目の矩形か」の数字で、それに応じた位置を得る
ofRectangle Ui::getLayerRowRect(int visibleRow) const {
	return layerList.rowRect(visibleRow);
}

void Ui::drawLayerPanel(
	const std::vector<std::unique_ptr<paintcore::Layer>> & layers,
	int selectedLayerId,
	const std::function<LayerType(int)> & getLayerType) {
	layerList.clampScroll(static_cast<int>(layers.size()));

	ofPushStyle();

	ofFill();
	ofEnableAlphaBlending();

	// ＋ボタン
	ofSetColor(65);
	ofDrawRectangle(layerAddRect);

	ofSetColor(255);
	ofDrawBitmapString(
		layerMenuOpen ? "Close" : "+",
		layerAddRect.x + 8,
		layerAddRect.y + 19);

	// 一覧の背景
	ofSetColor(35);
	ofDrawRectangle(layerList.bounds());

	if (layerMenuOpen) {
		// メニューを開いている間は、一覧の代わりに選択肢を表示
		const ofRectangle normalRect = getLayerRowRect(0);
		const ofRectangle glowRect = getLayerRowRect(1);

		ofSetColor(70);
		ofDrawRectangle(normalRect);
		ofDrawRectangle(glowRect);

		ofSetColor(255);
		ofDrawBitmapString(
			"Normal",
			normalRect.x + 8,
			normalRect.y + 27);

		ofDrawBitmapString(
			"Glow",
			glowRect.x + 8,
			glowRect.y + 27);
	} else {
		for (int row = 0; row < visibleLayerCount; ++row) {
			const int index = layerList.firstVisibleIndex() + row;

			if (index >= static_cast<int>(layers.size())) {
				break;
			}

			const paintcore::Layer & layer = *layers[index];
			const ofRectangle rect = getLayerRowRect(row);

			// 選択中の行を強調
			if (layer.id == selectedLayerId) {
				ofSetColor(65, 105, 155);
			} else {
				ofSetColor(60);
			}

			ofDrawRectangle(
				rect.x, rect.y,
				rect.width, rect.height - 2);

			ofSetColor(255);
			ofDrawBitmapString(
				layer.name,
				rect.x + 8,
				rect.y + 17);

			const std::string typeName = getLayerType(layer.id) == LayerType::Normal
				? "NormalLayer"
				: "GlowLayer";

			ofDrawBitmapString(
				typeName,
				rect.x + 8,
				rect.y + 34);

			const ofRectangle deleteRect = getLayerDeleteRect(row);

			// 最後の1枚は削 > 1除できないので、暗く表示する
			if (layers.size() > 1) {
				ofSetColor(150, 60, 60);
			} else {
				ofSetColor(65);
			}

			ofDrawRectangle(deleteRect);

			ofSetColor(255);
			ofDrawBitmapString(
				"x",
				deleteRect.x + 7,
				deleteRect.y + 15);
		}
	}

	ofPopStyle();
}

// クリックしたのがどのボタンだったかを判定し、actiontypeを返す関数
// レイヤーがクリックされていた場合draggedlayerに設定
LayerUiAction Ui::pressLayerPanel(
	int x,
	int y,
	const std::vector<std::unique_ptr<paintcore::Layer>> & layers) {
	LayerUiAction action;

	// 新しいクリックの開始時にリセットする
	layerList.cancelDrag();

	// ＋ボタンでメニューを開閉
	if (layerAddRect.inside(x, y)) {
		layerMenuOpen = !layerMenuOpen;
		return action;
	}

	if (layerMenuOpen) {
		if (getLayerRowRect(0).inside(x, y)) {
			action.type = LayerUiActionType::AddNormal;
		} else if (getLayerRowRect(1).inside(x, y)) {
			action.type = LayerUiActionType::AddGlow;
		}

		// 選択肢または外側を押したら閉じる
		layerMenuOpen = false;
		return action;
	}

	// 一覧の行をクリック
	for (int row = 0; row < visibleLayerCount; ++row) {
		const int index = layerList.firstVisibleIndex() + row;

		if (index >= static_cast<int>(layers.size())) {
			break;
		}

		if (getLayerDeleteRect(row).inside(x, y)) {
			if (layers.size() > 1) {
				action.type = LayerUiActionType::Delete;
				action.layerId = layers[index]->id;
			}

			return action;
		}

		if (getLayerRowRect(row).inside(x, y)) {
			action.type = LayerUiActionType::Select;
			action.layerId = layers[index]->id;

			layerList.beginDrag(action.layerId);

			return action;
		}
	}

	return action;
}

LayerUiAction Ui::releaseLayerPanel(
	int x,
	int y,
	int layerCount) {

	LayerUiAction action;

	const auto request = layerList.endDrag(x, y, layerCount);

	if (request.layerId == 0) {
		return action;
	}

	action.type = LayerUiActionType::Move;
	action.layerId = request.layerId;
	action.destinationIndex = request.destinationIndex;

	return action;
}

void Ui::scrollLayerPanel(float scrollY, int layerCount) {
	if (layerMenuOpen) {
		return;
	}

	layerList.scroll(scrollY, layerCount);
}

ofRectangle Ui::getLayerDeleteRect(int visibleRow) const {
	const ofRectangle rowRect = getLayerRowRect(visibleRow);

	return ofRectangle(
		rowRect.getRight() - 26,
		rowRect.y + 4,
		22,
		22);
}
