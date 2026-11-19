#include "NormalFilter.h"
#include "CanvasRenderer.h"

LayerType NormalFilter::getType() const {
	return LayerType::Normal;
}

void NormalFilter::setup(int width, int height) {

}

void NormalFilter::markDirty() {

}

// 通常キャンバスはただ受け取ったデータをそのまま表示する
void NormalFilter::draw(ofFbo& source, float time) {
	paintcore::CanvasRenderer::drawNormalImage(source);
}
