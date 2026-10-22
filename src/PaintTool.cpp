#include "PaintTool.h"
#include <cmath>
#include <utility>
#include <stdexcept>

PaintTool::PaintTool(int mainWidth, int mainHeight) {
	canvasWidth = mainWidth;
	canvasHeight = mainHeight;

	color = ofColor(0, 0, 0, 255);
	strokeSize = 0;

	ofFbo::Settings s;
	s.width = mainWidth;
	s.height = mainHeight;
	s.internalformat = GL_RGBA32F;

	preFbo2D.allocate(s);
	historySwapFbo.allocate(s);

	for (int i = 0; i < UNDO_CAPACITY; i++) {
		history[i].image.allocate(s);
	}
}

void PaintTool::drawLine(bool penSwitch, float eraserAlpha) {
	Layer & layer = getLayer(strokeLayerId);
	ofFbo & base = layer.canvas;

	ofFill();

	preFbo2D.begin();
	ofClear(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	base.draw(0, 0);

	if (penSwitch) {
		ofSetColor(color);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		ofSetColor(255, 255, 255, eraserAlpha);
		glBlendFuncSeparate(
			GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
			GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
	}

	for (int i = 0; i < points.size(); i++) {
		ofDrawCircle(points[i].x, points[i].y, strokeSize * 25);
	}

	preFbo2D.end();

	layer.filter->markDirty();
}

void PaintTool::drawCanvas(bool drawingFlag) {
	float time = ofGetElapsedTimef();

	for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
		Layer & layer = **it; // 配列の中の、ポインタの中の、Layer本体だから**it

		const bool isDrawingLayer = drawingFlag && layer.id == strokeLayerId;

		ofFbo & source = isDrawingLayer ? preFbo2D : layer.canvas;

		layer.filter->draw(source, time);
	}
}

void PaintTool::copyFbo(ofFbo & source, ofFbo & destination) {
	ofPushStyle();

	destination.begin();
	ofClear(0, 0, 0, 0);
	ofDisableAlphaBlending();
	ofSetColor(255, 255, 255, 255);

	source.draw(0, 0);

	destination.end();

	ofPopStyle();
}

void PaintTool::swapHistory(int index) {
	HistoryEntry & entry = history[index];

	if (entry.type == HistoryType::CanvasEdit) {
		Layer & layer = getLayer(entry.layerId);
		ofFbo & target = layer.canvas;

		copyFbo(target, historySwapFbo);
		copyFbo(entry.image, target);
		copyFbo(historySwapFbo, entry.image);

		layer.filter->markDirty();
		return;
	}

	if (entry.type == HistoryType::LayerAdded) {
		if (!entry.storedLayer) {
			// UNDO：追加されたレイヤーを履歴へ預ける
			int layerIndex = -1;

			for (int i = 0; i < static_cast<int>(layers.size()); ++i) {
				if (layers[i]->id == entry.layerId) {
					layerIndex = i;
					break;
				}
			}

			if (layerIndex == -1) {
				throw std::logic_error(
					"Added layer was not found during undo.");
			}

			// 現在の位置を記憶し、REDOで同じ場所へ戻す
			entry.layerIndex = layerIndex;

			entry.storedLayer = std::move(layers[layerIndex]);
			layers.erase(layers.begin() + layerIndex);

			selectedLayerId = entry.selectedBefore;
			nextLayerId = entry.nextIdBefore;
		} else {
			// REDO：預けていたレイヤーを配列へ戻す
			entry.storedLayer->filter->markDirty();

			layers.insert(
				layers.begin() + entry.layerIndex,
				std::move(entry.storedLayer));

			selectedLayerId = entry.selectedAfter;
			nextLayerId = entry.nextIdAfter;
		}
	}

	if (entry.type == HistoryType::LayerDeleted) {
		if (entry.storedLayer) {
			// UNDO：削除したレイヤーを元の位置へ戻す
			entry.storedLayer->filter->markDirty();

			layers.insert(
				layers.begin() + entry.layerIndex,
				std::move(entry.storedLayer));

			selectedLayerId = entry.selectedBefore;
		} else {
			// REDO：復元したレイヤーを再び取り除く
			int layerIndex = -1;

			for (int i = 0; i < static_cast<int>(layers.size()); ++i) {
				if (layers[i]->id == entry.layerId) {
					layerIndex = i;
					break;
				}
			}

			if (layerIndex == -1) {
				throw std::logic_error(
					"Deleted layer was not found during redo.");
			}

			// 次のUNDOで戻す位置を記憶する
			entry.layerIndex = layerIndex;

			entry.storedLayer = std::move(layers[layerIndex]);
			layers.erase(layers.begin() + layerIndex);

			selectedLayerId = entry.selectedAfter;
		}
	}
}

void PaintTool::saveUndo(Layer& layer) {
	HistoryEntry & entry = prepareHistory();

	entry.type = HistoryType::CanvasEdit;
	entry.layerId = layer.id;

	copyFbo(layer.canvas, entry.image);

	finishHistory();
}

void PaintTool::undo() {
	if (undoCount <= 0 || strokeLayerId !=0) {
		return;
	}

	undoWriteIndex = (undoWriteIndex + UNDO_CAPACITY - 1) % UNDO_CAPACITY;

	swapHistory(undoWriteIndex);

	undoCount--;
	redoCount++;
}

void PaintTool::redo() {
	if (redoCount <= 0 || strokeLayerId != 0) {
		return;
	}

	swapHistory(undoWriteIndex);

	undoWriteIndex = (undoWriteIndex + 1) % UNDO_CAPACITY;

	undoCount++;
	redoCount--;
}

void PaintTool::deleteAll() {
	if (strokeLayerId != 0) {
		return;
	}

	Layer & layer = getLayer(selectedLayerId);

	saveUndo(layer);

	layer.canvas.begin();
	ofClear(0, 0, 0, 0);
	layer.canvas.end();

	layer.filter->markDirty();
}

int PaintTool::addLayer(LayerType type, bool recordHistory) {
	if (strokeLayerId != 0) {
		return 0;
	}

	std::unique_ptr<LayerFilter> filter;

	switch (type) {
	case LayerType::Normal:
		filter = std::make_unique<NormalFilter>();
		break;

	case LayerType::Glow:
		filter = std::make_unique<GlowFilter>();
		break;

	default:
		throw std::invalid_argument("Unsupported layer type.");
	}

	// IDと表示名を決める
	const int id = nextLayerId;
	const std::string name = "Layer" + std::to_string(id);

	// レイヤー本体を作る
	auto layer = std::make_unique<Layer>(
		id,
		name,
		canvasWidth,
		canvasHeight,
		std::move(filter));

	int insertIndex = 0;

	for (int i = 0; i < static_cast<int>(layers.size()); ++i) {
		if (layers[i] ->id == selectedLayerId) {
			insertIndex = i;
			break;
		}
	}

	if (recordHistory) {
		HistoryEntry & entry = prepareHistory();

		entry.type = HistoryType::LayerAdded;
		entry.layerId = id;
		entry.layerIndex = insertIndex;
		entry.selectedBefore = selectedLayerId;
		entry.selectedAfter = id;
		entry.nextIdBefore = nextLayerId;
		entry.nextIdAfter = nextLayerId + 1;
	}

    layers.insert(
		layers.begin() + insertIndex,
		std::move(layer));

	++nextLayerId;
	selectedLayerId = id;

	if (recordHistory) {
		finishHistory();
	}

	return id;
}

Layer& PaintTool::getLayer(int layerId) {
	for (auto& layer : layers) {
		if (layer->id == layerId) {
			return *layer;
		}
	}

    throw std::out_of_range("Layer ID was not found.");
}

void PaintTool::commitStroke() {
	Layer & layer = getLayer(strokeLayerId);

	saveUndo(layer);
	copyFbo(preFbo2D, layer.canvas);
	layer.filter->markDirty();

	points.clear();
	strokeLayerId = 0;
}

void PaintTool::moveLayer(int layerId, int destinationIndex) {
	// 線を描いている途中では並べ替えない
	if (strokeLayerId != 0) {
		return;
	}

	const int layerCount = static_cast<int>(layers.size());

	if (destinationIndex < 0 || destinationIndex >= layerCount) {
		return;
	}

	// 移動するレイヤーの、現在の配列位置を調べる
	int sourceIndex = -1;

	for (int i = 0; i < layerCount; ++i) {
		if (layers[i]->id == layerId) {
			sourceIndex = i;
			break;
		}
	}

	if (sourceIndex == -1 || sourceIndex == destinationIndex) {
		return;
	}

	// 移動対象の所有権を、いったんローカル変数へ移す
	auto movingLayer = std::move(layers[sourceIndex]);

	// 元の配列位置を取り除く
	layers.erase(layers.begin() + sourceIndex);

	// 移動後の位置へ挿入する
	layers.insert(
		layers.begin() + destinationIndex,
		std::move(movingLayer));
}

void PaintTool::deleteLayer(int layerId) {
	// 描画中と、最後の1枚になった場合は削除しない
	if (strokeLayerId != 0 || layers.size() <= 1) {
		return;
	}

	const int layerCount = static_cast<int>(layers.size());
	int deleteIndex = -1;

	for (int i = 0; i < layerCount; ++i) {
		if (layers[i]->id == layerId) {
			deleteIndex = i;
			break;
		}
	}

	if (deleteIndex == -1) {
		return;
	}

	// 削除後に、どのレイヤーを選択するか決める
	int selectedAfter = selectedLayerId;

	if (selectedLayerId == layerId) {
		if (deleteIndex + 1 < layerCount) {
			selectedAfter = layers[deleteIndex + 1]->id;
		} else {
			selectedAfter = layers[deleteIndex - 1]->id;
		}
	}

	// 削除操作を履歴に記録する
	HistoryEntry & entry = prepareHistory();

	entry.type = HistoryType::LayerDeleted;
	entry.layerId = layerId;
	entry.layerIndex = deleteIndex;
	entry.selectedBefore = selectedLayerId;
	entry.selectedAfter = selectedAfter;

    // レイヤー本体を履歴へ預けてから、配列の枠を取り除く
	entry.storedLayer = std::move(layers[deleteIndex]);
	layers.erase(layers.begin() + deleteIndex);

    selectedLayerId = selectedAfter;

	finishHistory();
}

// 操作による履歴制作の前準備
// REDOができなくなるのでその分のレイヤーを破棄
// 余計な情報がうっかり持ち込まれないように、historyの次のインデックスを初期化
HistoryEntry& PaintTool::prepareHistory() {
	// 新しい操作を始めるため、REDO側の履歴を破棄する
	for (int i = 0; i < redoCount; ++i) {
		const int index = (undoWriteIndex + i) % UNDO_CAPACITY;
		history[index].storedLayer.reset();
	}

	redoCount = 0;

	HistoryEntry & entry = history[undoWriteIndex];

	// 上書きする履歴が預かっていたレイヤーを開放する
	entry.storedLayer.reset();

    entry.type = HistoryType::CanvasEdit;
	entry.layerId = 0;
	entry.layerIndex = 0;
	entry.selectedBefore = 0;
	entry.selectedAfter = 0;
	entry.nextIdBefore = 0;
	entry.nextIdAfter = 0;

	return entry;
}

void PaintTool::finishHistory() {
	undoWriteIndex = (undoWriteIndex + 1) % UNDO_CAPACITY;

	if (undoCount < UNDO_CAPACITY) {
		undoCount++;
	}
}

void PaintTool::clearHistory() {
	for (int i = 0; i < UNDO_CAPACITY; ++i) {
		history[i].storedLayer.reset();
	}

	undoWriteIndex = 0;
	undoCount = 0;
	redoCount = 0;
}
