#pragma once


#include "ofMain.h"
#include "NormalFilter.h"
#include "GlowFilter.h"
#include "PaintLayer.h"
#include "PaintEngine.h"
#include "HistoryAction.h"
#include "HistoryManager.h"

#include <memory>
#include <vector>
#include <unordered_map>

enum class HistoryType {
	CanvasEdit,
	LayerAdded,
	LayerDeleted
};

class PaintTool;

struct HistoryEntry : public paintcore::HistoryAction<PaintTool> {
	void undo(PaintTool & tool) override;
	void redo(PaintTool & tool) override;
	void reset();

	HistoryType type = HistoryType::CanvasEdit;
	int layerId = 0;
	ofFbo image;

	// UNDOで取り除いたレイヤーを預かる
	std::unique_ptr<paintcore::Layer> storedLayer;
	std::unique_ptr<LayerFilter> storedFilter;

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

	ofFbo historySwapFbo;

	static const int UNDO_CAPACITY = 20;

	PaintTool(int mainWidth, int mainHeight);
	void drawLine(bool penSwith, float eraserAlpha);
	void drawCanvas(bool drawingFlag);
	void saveUndo(paintcore::Layer & layer);
	void undo();
	void redo();
	void deleteAll();

	float strokeSize;

	int selectedLayerId = 0;
	int nextLayerId = 1;
	int canvasWidth = 0;
	int canvasHeight = 0;
	// 現在描いている線の保存先。0は描画していない状態
	int strokeLayerId = 0;

	// レイヤーIDから現在の配列位置を返す。見つからなければ-1。
	int findLayerIndex(int layerId) const;

	// IDからレイヤー本体やフィルターを取得する
	paintcore::Layer & getLayer(int layerId);
	LayerFilter & getFilter(int layerId);
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

	GlowFilter * getSelectedGlowFilter();

	void beginStroke(const glm::vec2 & position);
	void addStrokePoint(const glm::vec2 & position);
	void finishStroke(const glm::vec2 & position);

	paintcore::PaintEngine engine;
	GlowFilter * getGlowFilter(int layerId);

private:
	void storeLayerHistory(HistoryEntry & entry, int layerIndex);
	void restoreLayerFromHistory(HistoryEntry & entry);
	// idというキーを使ってフィルターを検索するための連想配列
	std::unordered_map<int, std::unique_ptr<LayerFilter>> filters;

	// HistoryEntryに対して、このクラスのプライベートメンバ変数にアクセスを許可
	friend struct HistoryEntry;

	void swapCanvasHistory(HistoryEntry & entry);
	void takeHistoryLayer(HistoryEntry & entry);
	paintcore::HistoryManager<HistoryEntry, UNDO_CAPACITY> historyManager;
};
