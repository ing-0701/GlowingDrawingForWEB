#pragma once

#include "ofMain.h"

// このクラスは様々なフィルター以前の抽象クラス

// レイヤーの種類
enum class LayerType {
	Normal,
	Glow
};

// 全てのフィルター共通の機能
class LayerFilter {
	public :
		virtual ~LayerFilter() = default;

		// このフィルターの種類を返す
		virtual LayerType getType() const = 0;

		// 必要なFBOやシェーダーを準備する
		virtual void setup(int width, int height) = 0;

		// 元画像が変わったことを知らせる
		virtual void markDirty() = 0;

		//元画像を使って加工結果を画面に表示
		virtual void draw(ofFbo & source, float time) = 0;
};
