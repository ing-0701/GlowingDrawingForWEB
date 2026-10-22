#pragma once

#include "ofMain.h"
#include "LayerFilter.h"

#include <memory>
#include <string>

class Layer {
public:
	// 識別番号
	const int id;

	// 表示名
	std::string name;

	// キャンバス本体
	ofFbo canvas;

	// 種類ごとの専用表示フィルター
	std::unique_ptr<LayerFilter> filter;

	Layer(
		int layerId,
		const std::string & layerName,
		int width,
		int height,
		std::unique_ptr<LayerFilter> layerFilter
	);
};
