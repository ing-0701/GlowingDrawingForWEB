#pragma once

#include "LayerFilter.h"

class NormalFilter : public LayerFilter {
public:
	LayerType getType() const override;

	void setup(int width, int height) override;
	void markDirty() override;
	void draw(ofFbo & source, float time) override;
};
