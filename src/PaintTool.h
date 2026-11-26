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
    void newDocument() { PaintSession::newDocument(std::make_unique<NormalFilter>()); }

    // Concrete member wrappers also keep ofxGui's listener member-pointer types unchanged.
    void undo() { PaintSession::undo(); }
    void redo() { PaintSession::redo(); }
};
