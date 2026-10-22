#include "Layer.h"

#include <stdexcept>
#include <utility>

Layer::Layer(
	int layerId,
	const std::string & layerName,
	int width,
	int height,
	std::unique_ptr<LayerFilter> layerFilter)
	: id(layerId)
	, name(layerName)
	, filter(std::move(layerFilter)) {
	if (!filter) {
		throw std::invalid_argument(
			"Layer requires a filter.");
	}

	// 描画本体を用意
	ofFbo::Settings settings;
	settings.width = width;
	settings.height = height;
	settings.internalformat = GL_RGBA32F;

	canvas.allocate(settings);

	// 初期状態は透明にする
	canvas.begin();
	ofClear(0, 0, 0, 0);
	canvas.end();

	// フィルター準備
	filter->setup(width, height);

	//最初の表示時に加工結果を作れるようにする
	filter->markDirty();
}
