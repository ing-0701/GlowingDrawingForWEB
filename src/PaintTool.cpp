#include "PaintTool.h"
#include <stdexcept>

void PaintTool::setup() {
    PaintSession::setup(std::make_unique<NormalFilter>());
}

int PaintTool::addLayer(LayerType type, bool recordHistory) {
    switch (type) {
    case LayerType::Normal:
        return PaintSession::addLayer(std::make_unique<NormalFilter>(), recordHistory);
    case LayerType::Glow:
        return PaintSession::addLayer(std::make_unique<GlowFilter>(), recordHistory);
    default:
        throw std::invalid_argument("Unsupported layer type.");
    }
}

LayerFilter& PaintTool::getFilter(int layerId) {
    return dynamic_cast<LayerFilter&>(getExtension(layerId));
}

GlowFilter* PaintTool::getGlowFilter(int layerId) {
    if (findLayerIndex(layerId) < 0) return nullptr;
    return dynamic_cast<GlowFilter*>(&getExtension(layerId));
}

GlowFilter* PaintTool::getSelectedGlowFilter() {
    return getGlowFilter(getSelectedLayerId());
}
