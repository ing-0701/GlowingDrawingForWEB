#include "Ui.h"
#include <algorithm>

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
	penSize.setup("penSize", 0.1, 0.05, 1.0);
	penSize.setSize(256, 30);
	penSize.setPosition(mainWidth, 258 + 32 * 1);
	penSwitch.setup("pen/eraser", true);
	penSwitch.setSize(100, 30);
	penSwitch.setPosition(mainWidth, 258 + 32 * 4);
	penAlpha.setup("penAlpha", 1.0, 0.01, 1.0);
	penAlpha.setSize(256, 30);
	penAlpha.setPosition(mainWidth, 258 + 32 * 2);
	eraserSize.setup("eraserSize", 0.1, 0.05, 1.0);
	eraserSize.setSize(256, 30);
	eraserSize.setPosition(mainWidth, 258 + 32 * 1);
	eraserAlpha.setup("eraserAlpha", 1.0, 0.01, 1.0);
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
	glowSigma.setup("sigma", 2.4f, 1.2f, 4.8f);
	glowSigma.setSize(256, 30);
	glowSigma.setPosition(mainWidth, 606);
	glowStrength.setup("glowStrength", 3.0f, 1.5f, 6.0f);
	glowStrength.setSize(256, 30);
	glowStrength.setPosition(mainWidth, 638);
	glowPeriod.setup("period", 1.0f, 0.3f, 3.0f);
	glowPeriod.setSize(256, 30);
	glowPeriod.setPosition(mainWidth, 670);

	// 起動時はNormalレイヤーなので、操作を受け付けない
	glowSigma.unregisterMouseEvents();
	glowStrength.unregisterMouseEvents();
	glowPeriod.unregisterMouseEvents();
#endif // !__EMSCRIPTEN__

	pickerPix.allocate(256, 256, OF_PIXELS_RGBA);
	pickerTex.allocate(pickerPix);

	layerAddRect.set(
		mainWidth + 104, 354,
		148, 28);

	layerListRect.set(
		mainWidth + 104, 386,
		148, layerRowHeight * visibleLayerCount);
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
	undo.draw();
	redo.draw();
	deleteAll.draw();
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
	return ofRectangle(
		layerListRect.x,
		layerListRect.y + visibleRow * layerRowHeight,
		layerListRect.width,
		layerRowHeight);
}

void Ui::drawLayerPanel(
	const std::vector<std::unique_ptr<Layer>> & layers,
	int selectedLayerId) {
	const int maxFirst = std::max(
		0,
		static_cast<int>(layers.size()) - visibleLayerCount);

	firstVisibleLayer = std::clamp(
		firstVisibleLayer,
		0,
		maxFirst);

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
	ofDrawRectangle(layerListRect);

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
			const int index = firstVisibleLayer + row;

			if (index >= static_cast<int>(layers.size())) {
				break;
			}

			const Layer & layer = *layers[index];
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

			const std::string typeName = layer.filter->getType() == LayerType::Normal
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
	const std::vector<std::unique_ptr<Layer>> & layers) {
	LayerUiAction action;

	// 新しいクリックの開始時にリセットする
	draggedLayerId = 0;

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
		const int index = firstVisibleLayer + row;

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

			draggedLayerId = action.layerId;

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

	// 押していたレイヤーを取り出し、ドラッグ状態を解除する
	const int layerId = draggedLayerId;
	draggedLayerId = 0;

	// レイヤーの行から始まった操作でなければ何もしない
	if (layerId == 0) {
		return action;
	}

	// 一覧の外で離した場合は、並べ替えを取り消す
	if (!layerListRect.inside(x, y)) {
		return action;
	}

	// 一覧の上から何行目で離したか
	const int row = static_cast<int>(
		(y - layerListRect.y) / layerRowHeight);

	// スクロール位置を加え、配列全体での位置に変換する
	const int destinationIndex = firstVisibleLayer + row;

	// レイヤーが存在しない空白部分なら何もしない
	if (destinationIndex >= layerCount) {
		return action;
	}

	action.type = LayerUiActionType::Move;
	action.layerId = layerId;
	action.destinationIndex = destinationIndex;

	return action;
}

void Ui::scrollLayerPanel(float scrollY, int layerCount) {
	if (layerMenuOpen) {
		return;
	}

	if (scrollY > 0) {
		--firstVisibleLayer;
	} else if (scrollY < 0) {
		++firstVisibleLayer;
	}

	const int maxFirst = std::max(0, layerCount - visibleLayerCount);

	firstVisibleLayer = std::clamp(firstVisibleLayer, 0, maxFirst);
}

ofRectangle Ui::getLayerDeleteRect(int visibleRow) const {
	const ofRectangle rowRect = getLayerRowRect(visibleRow);

	return ofRectangle(
		rowRect.getRight() - 26,
		rowRect.y + 4,
		22,
		22);
}
