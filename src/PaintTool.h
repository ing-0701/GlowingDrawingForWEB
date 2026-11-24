#pragma once

#include "PaintSession.h"
#include "NormalFilter.h"
#include "GlowFilter.h"

// GlowingDrawing only chooses layer effects. Standard painting/history lives in PaintCore.
class PaintTool : public paintcore::PaintSession {
public:
    using paintcore::PaintSession::PaintSession;
    void setup();
    int addLayer(LayerType type, bool recordHistory = true);
    LayerFilter& getFilter(int layerId);
    GlowFilter* getGlowFilter(int layerId);
    GlowFilter* getSelectedGlowFilter();
    // Keep GlowingDrawing's existing behavior: reordering is not an Undo step.
    void moveLayer(int id, int index) { PaintSession::moveLayer(id, index, false); }
    void newDocument() { PaintSession::newDocument(std::make_unique<NormalFilter>()); }
    bool loadProject(const std::string& path);

    // Concrete member wrappers also keep ofxGui's listener member-pointer types unchanged.
    void undo() { PaintSession::undo(); }
    void redo() { PaintSession::redo(); }
};
