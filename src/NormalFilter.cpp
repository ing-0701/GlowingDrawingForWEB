#include "NormalFilter.h"

LayerType NormalFilter::getType() const {
	return LayerType::Normal;
}

void NormalFilter::setup(int width, int height) {

}

void NormalFilter::markDirty() {

}

// 通常キャンバスはただ受け取ったデータをそのまま表示する
void NormalFilter::draw(ofFbo& source, float time) {
	ofEnableAlphaBlending();
	ofSetColor(255, 255, 255, 255);

	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	source.draw(0, 0);
}
