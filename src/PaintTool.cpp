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

bool PaintTool::loadProject(const std::string& path) {
    return PaintSession::loadProject(path, [](const ofJson& state) -> std::unique_ptr<paintcore::LayerExtension> {
        const auto type = state.at("type").get<std::string>();
        if (type == "Normal") return std::make_unique<NormalFilter>();
        if (type == "Glow") {
            auto glow = std::make_unique<GlowFilter>();
            glow->setParameters(state.at("sigma"), state.at("strength"), state.at("period"));
            return glow;
        }
        return nullptr;
    });
}
