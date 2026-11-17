#include <cmath>
#include <utility>
#include <stdexcept>

#include "PaintTool.h"
#include "CanvasRenderer.h"

PaintTool::PaintTool(int mainWidth, int mainHeight) {
	canvasWidth = mainWidth;
	canvasHeight = mainHeight;

	color = ofColor(0, 0, 0, 255);
	strokeSize = 0;

	ofFbo::Settings s;
	s.width = mainWidth;
	s.height = mainHeight;
	s.internalformat = GL_RGBA32F;
#ifdef __EMSCRIPTEN__
	s.textureTarget = GL_TEXTURE_2D;
#endif // __EMSCRIPTEN__


	engine.allocateStrokeBuffers(s);
	historySwapFbo.allocate(s);

	historyManager.initializeEntries([&s](HistoryEntry& entry) {
		entry.image.allocate(s);
	});
	
}

void PaintTool::beginStroke(const glm::vec2 & position) {
	strokeLayerId = selectedLayerId;
	paintcore::Layer & layer = getLayer(strokeLayerId);
	engine.beginStroke(position, layer.canvas);
}

void PaintTool::addStrokePoint(const glm::vec2 & position) {
	engine.addStrokePoint(position);
}

void PaintTool::finishStroke(const glm::vec2 & position) {
	engine.finishStroke(position);
}

void PaintTool::drawLine(bool penSwitch, float eraserAlpha) {
	paintcore::Layer & layer = getLayer(strokeLayerId);

	const paintcore::BrushSettings settings {
		strokeSize,
		color,
		penSwitch,
		eraserAlpha
	};

	engine.drawStroke(settings);
	getFilter(layer.id).markDirty();
}

void PaintTool::drawCanvas(bool drawingFlag) {
	const float time = ofGetElapsedTimef();
	const int previewLayerId = drawingFlag ? strokeLayerId : 0;

	// 第三引数はラムダ式
	// 目的のレイヤーに対応するフィルターに応じて描画関数を呼ぶ
	// thisは今回使いたいgetFilter()やdraw()を持つオブジェクト
	// timeはこのdrawCanvas内で定義されたローカル変数なので明示が必要
	paintcore::CanvasRenderer::draw(
		engine,
		previewLayerId,
		[this, time](const paintcore::Layer & layer, ofFbo & source) {
			getFilter(layer.id).draw(source, time);
		});
}

void PaintTool::storeLayerHistory(HistoryEntry& entry, int layerIndex) {
	entry.layerIndex = layerIndex;
	entry.storedLayer = engine.takeLayerAt(layerIndex);

	
	const int id = entry.storedLayer->id;
	entry.storedFilter = std::move(filters.at(id));
	filters.erase(id);
}

void PaintTool::restoreLayerFromHistory(HistoryEntry& entry) {
	const int id = entry.storedLayer->id;

	entry.storedFilter->markDirty();
	filters.emplace(id, std::move(entry.storedFilter));

	engine.insertLayerAt(entry.layerIndex, std::move(entry.storedLayer));
}

void PaintTool::swapCanvasHistory(HistoryEntry& entry) {
	paintcore::Layer & layer = getLayer(entry.layerId);
	ofFbo & target = layer.canvas;

	paintcore::copyFbo(target, historySwapFbo);
	paintcore::copyFbo(entry.image, target);
	paintcore::copyFbo(historySwapFbo, entry.image);

	getFilter(layer.id).markDirty();
}

// レイヤーの位置を調べてから履歴を保存する関数
// レイヤー削除時はすでに位置がわかってるから直接storeLayerHistoryを呼ぶ
void PaintTool::takeHistoryLayer(HistoryEntry& entry) {
	const int layerIndex = findLayerIndex(entry.layerId);

	if (layerIndex == -1) {
		throw std::logic_error("History layer was not found.");
	}

	storeLayerHistory(entry, layerIndex);
}

void HistoryEntry::undo(PaintTool& tool) {
	switch (type) {
	case HistoryType::CanvasEdit:
		tool.swapCanvasHistory(*this);
		break;

	case HistoryType::LayerAdded:
		tool.takeHistoryLayer(*this);
		tool.selectedLayerId = selectedBefore;
		tool.nextLayerId = nextIdBefore;
		break;

	case HistoryType::LayerDeleted:
		tool.restoreLayerFromHistory(*this);
		tool.selectedLayerId = selectedBefore;
		break;
	}
}

void HistoryEntry::redo(PaintTool & tool) {
	switch (type) {
	case HistoryType::CanvasEdit:
		tool.swapCanvasHistory(*this);
		break;

	case HistoryType::LayerAdded:
		tool.restoreLayerFromHistory(*this);
		tool.selectedLayerId = selectedAfter;
		tool.nextLayerId = nextIdAfter;
		break;

	case HistoryType::LayerDeleted:
		tool.takeHistoryLayer(*this);
		tool.selectedLayerId = selectedAfter;
		break;
	}
}

void HistoryEntry::reset() {
	storedLayer.reset();
	storedFilter.reset();

	type = HistoryType::CanvasEdit;
	layerId = 0;
	layerIndex = 0;
	selectedBefore = 0;
	selectedAfter = 0;
	nextIdBefore = 0;
	nextIdAfter = 0;
}

void PaintTool::saveUndo(paintcore::Layer & layer) {
	HistoryEntry & entry = prepareHistory();

	entry.type = HistoryType::CanvasEdit;
	entry.layerId = layer.id;

	paintcore::copyFbo(layer.canvas, entry.image);

	finishHistory();
}

void PaintTool::undo() {
	if (strokeLayerId != 0) {
		return;
	}

	historyManager.undo(*this);
}

void PaintTool::redo() {
	if (strokeLayerId != 0) {
		return;
	}

	historyManager.redo(*this);
}

void PaintTool::deleteAll() {
	if (strokeLayerId != 0) {
		return;
	}

	paintcore::Layer & layer = getLayer(selectedLayerId);

	saveUndo(layer);

	layer.canvas.begin();
	ofClear(0, 0, 0, 0);
	layer.canvas.end();

	getFilter(layer.id).markDirty();
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
	auto layer = std::make_unique<paintcore::Layer>(
		id,
		name,
		canvasWidth,
		canvasHeight);

	filter->setup(canvasWidth, canvasHeight);
	filter->markDirty();

	// 現在選択中のレイヤーの上に新規レイヤーを作る
	// 選択中のレイヤーがないときは基本起動時につき最初のレイヤーを生成
	const int selectedIndex = findLayerIndex(selectedLayerId);
	const int insertIndex = selectedIndex >= 0 ? selectedIndex : 0;

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

	filters.emplace(id, std::move(filter));

    engine.insertLayerAt(insertIndex, std::move(layer));

	++nextLayerId;
	selectedLayerId = id;

	if (recordHistory) {
		finishHistory();
	}

	return id;
}

int PaintTool::findLayerIndex(int layerId) const {
	return engine.findLayerIndex(layerId);
}

paintcore::Layer & PaintTool::getLayer(int layerId) {
	return engine.getLayer(layerId);
}

LayerFilter& PaintTool::getFilter(int layerId) {
	return *filters.at(layerId);
}

void PaintTool::commitStroke() {
	paintcore::Layer & layer = getLayer(strokeLayerId);

	saveUndo(layer);
	engine.commitStroke(layer.canvas);
	getFilter(layer.id).markDirty();

	strokeLayerId = 0;
}

void PaintTool::moveLayer(int layerId, int destinationIndex) {
	if (strokeLayerId != 0) {
		return;
	}

	engine.moveLayer(layerId, destinationIndex);
}

void PaintTool::deleteLayer(int layerId) {
	const auto & layers = engine.getLayers();

	// 描画中と、最後の1枚になった場合は削除しない
	if (strokeLayerId != 0 || layers.size() <= 1) {
		return;
	}

	const int layerCount = static_cast<int>(layers.size());
	const int deleteIndex = findLayerIndex(layerId);

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
	entry.selectedBefore = selectedLayerId;
	entry.selectedAfter = selectedAfter;

    // 削除時の位置を記録し、レイヤー本体を履歴へ預ける
	storeLayerHistory(entry, deleteIndex);

    selectedLayerId = selectedAfter;

	finishHistory();
}

// 操作による履歴制作の前準備
// REDOができなくなるのでその分のレイヤーを破棄
// 余計な情報がうっかり持ち込まれないように、historyの次のインデックスを初期化
HistoryEntry& PaintTool::prepareHistory() {
	return historyManager.prepare();
}

void PaintTool::finishHistory() {
	historyManager.finish();
}

void PaintTool::clearHistory() {
	historyManager.clear();
}

GlowFilter* PaintTool::getGlowFilter(int layerId) {
	if (findLayerIndex(layerId) == -1) {
		return nullptr;
	}

	LayerFilter& filter = getFilter(layerId);
	if (filter.getType() != LayerType::Glow) {
		return nullptr;
	}

	return static_cast<GlowFilter *>(&filter);
}

GlowFilter* PaintTool::getSelectedGlowFilter() {
	return getGlowFilter(selectedLayerId);
}
