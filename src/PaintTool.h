#pragma once


#include "ofMain.h"
#include "NormalFilter.h"
#include "GlowFilter.h"

#include "Layer.h"

#include <memory>
#include <vector>

enum class HistoryType {
	CanvasEdit,
	LayerAdded,
	LayerDeleted
};

struct HistoryEntry {
	HistoryType type = HistoryType::CanvasEdit;
	int layerId = 0;
	ofFbo image;

	// UNDOで取り除いたレイヤーを預かる
	std::unique_ptr<Layer> storedLayer;

	// 配列内の位置
	int layerIndex = 0;

	// 操作前後の選択状態
	int selectedBefore = 0;
	int selectedAfter = 0;

	// 操作前後の、次に発行するレイヤー番号
	int nextIdBefore = 0;
	int nextIdAfter = 0;
};

class PaintTool {
public:
	ofColor color;
	vector<glm::vec2> points;
	ofFbo preFbo2D;
	ofFbo historySwapFbo;
	static const int UNDO_CAPACITY = 20;
	HistoryEntry history[UNDO_CAPACITY];
	int undoWriteIndex = 0;
	int undoCount = 0;
	int redoCount = 0;

	PaintTool(int mainWidth, int mainHeight);

	void drawLine(bool penSwith, float eraserAlpha);
	void drawCanvas(bool drawingFlag);
	void copyFbo(ofFbo & source, ofFbo & destination);
	void swapHistory(int index);
	void saveUndo(Layer& layer);
	void undo();
	void redo();
	void deleteAll();

	float strokeSize;

	std::vector<std::unique_ptr<Layer>> layers;
	int selectedLayerId = 0;
	int nextLayerId = 1;
	int canvasWidth = 0;
	int canvasHeight = 0;
	// 現在描いている線の保存先。0は描画していない状態
	int strokeLayerId = 0;

	// IDからレイヤー本体を取得する
	Layer & getLayer(int layerId);
	// 描画途中の画像を、対象レイヤーへ確定する
	void commitStroke();

	// レイヤーを追加し、IDを返す
	int addLayer(LayerType type, bool recordHistory = true);

	// 指定したレイヤーを、移動後の配列位置へ移す
	void moveLayer(int layerId, int destinationIndex);

	void deleteLayer(int layerId);

	HistoryEntry & prepareHistory();
	void finishHistory();
	void clearHistory();
};
