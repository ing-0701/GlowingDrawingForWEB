#include "PaintTool.h"
#include "PaintEditor.h"
#include "../addons/ofxPaintCore/example-basic/src/ofApp.h"
#include <fstream>
#include <stdexcept>

namespace {
std::string testPath(const char* name) { return ofToDataPath(std::string("../../obj/paintcore-tests/")+name, true); }
void require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
ofFloatPixels pixels(paintcore::PaintSession& s, int id) {
    ofFloatPixels p; s.getLayer(id).canvas.readToPixels(p); return p;
}
void equal(const ofFloatPixels& a, const ofFloatPixels& b, const char* label) {
    require(a.size() == b.size(), label);
    for (std::size_t i=0; i<a.size(); ++i) require(std::abs(a[i]-b[i]) < 0.00001f, label);
}
void stroke(paintcore::PaintSession& s) {
    require(s.pointerPressed({10,10}), "press");
    for (int i=11; i<55; ++i) s.pointerDragged({i,20});
    s.pointerReleased(); // No draw() between press/release: must still commit latest preview.
}
void tests() {
    paintcore::PaintSettings config; config.width=64; config.height=64; config.historyCapacity=4;
    paintcore::PaintSession s(config); s.setup();
    require(s.getSelectedLayerId()==1 && s.engine.getLayers().size()==1, "initial layer");
    const auto empty=pixels(s,1);
    paintcore::BrushSettings initialBrush;
    initialBrush.size = 0.15f;
    initialBrush.eraserSize = 0.15f;
    initialBrush.color = ofColor(200,100,50,128);
    initialBrush.isPen = true;
    initialBrush.eraserAlpha = 255.0f;
    s.setBrushSettings(initialBrush);
    stroke(s);
    const auto painted=pixels(s,1);
    require(painted.getColor(10,10).a>0, "stroke without a frame");
    s.undo(); equal(pixels(s,1),empty,"undo stroke");
    s.redo(); equal(pixels(s,1),painted,"redo stroke");
    auto eraser=s.getBrushSettings(); eraser.isPen=false; eraser.eraserAlpha=128;
    s.setBrushSettings(eraser); stroke(s);
    require(pixels(s,1).getColor(10,10).a<painted.getColor(10,10).a,"transparent eraser");
    s.undo(); equal(pixels(s,1),painted,"undo eraser");
    s.deleteAll(); equal(pixels(s,1),empty,"clear"); s.undo(); equal(pixels(s,1),painted,"undo clear");
    const int two=s.addLayer(); require(two==2,"next layer id");
    s.undo(); require(s.engine.getLayers().size()==1,"undo add");
    s.redo(); require(s.getSelectedLayerId()==2,"redo add selection");
    paintcore::BrushSettings redBrush;
    redBrush.size = 0.1f;
    redBrush.eraserSize = 0.1f;
    redBrush.color = ofColor::red;
    redBrush.isPen = true;
    redBrush.eraserAlpha = 255.0f;
    s.setBrushSettings(redBrush); stroke(s);
    auto twoImage=pixels(s,two);
    s.deleteLayer(two); require(s.getSelectedLayerId()==1,"delete selection");
    s.undo(); equal(pixels(s,two),twoImage,"restore deleted pixels");
    s.undo(); equal(pixels(s,two),empty,"undo stroke before deletion");
    s.redo(); s.redo(); require(s.engine.getLayers().size()==1,"redo deletion");
    s.deleteLayer(1); require(s.engine.getLayers().size()==1,"last layer guard");
    s.newDocument();
    for (int i=0; i<8; ++i) s.addLayer();
    for (int i=0; i<8; ++i) s.undo();
    require(s.engine.getLayers().size()==5,"history capacity");
    for (int i=0; i<4; ++i) s.redo();
    require(s.engine.getLayers().size()==9,"ring redo");
    s.undo(); require(s.addLayer()==9,"id rewind and branching");
    require(!s.canRedo(),"successful operation discards redo");
    s.newDocument(); s.addLayer(); s.undo(); s.addLayer({},false);
    require(!s.canUndo() && !s.canRedo(),"unrecorded add invalidates old actions");
    s.newDocument();
    const int id=s.addLayer();
    s.clearHistory();
    s.moveLayer(id,1);
    require(s.findLayerIndex(id)==1,"move");
    require(!s.canUndo(),"reorder does not create history");
    s.setBrushSettings(redBrush); stroke(s);
    s.moveLayer(id,0);
    s.undo();
    equal(pixels(s,id),empty,"undo stroke targets layer after reorder");
    require(s.findLayerIndex(id)==0,"undo does not undo reorder");

    PaintTool glow(config); glow.setup();
    const int glowId=glow.addLayer(LayerType::Glow);
    auto* effect=glow.getGlowFilter(glowId);
    require(effect!=nullptr,"Glow extension");
    effect->setParameters(3.0f,5.0f,0.7f);
    stroke(glow); auto glowImage=pixels(glow,glowId);
    glow.deleteLayer(glowId); glow.undo();
    require(glow.getGlowFilter(glowId)==effect,"same effect object restored");
    require(std::abs(effect->getPeriod()-0.7f)<0.0001f,"effect parameters preserved");
    equal(pixels(glow,glowId),glowImage,"Glow pixels restored");

    // Replay unchanged input through legacy low-level drawing and new session, compare every texel.
    paintcore::PaintEngine legacy; legacy.setup(config);
    paintcore::PaintSession modern(config); modern.setup();
    paintcore::BrushSettings brush;
    brush.size = 0.1f;
    brush.eraserSize = 0.1f;
    brush.color = ofColor(100,150,200,90);
    brush.isPen = true;
    brush.eraserAlpha = 255.0f;
    modern.setBrushSettings(brush); modern.pointerPressed({5,5});
    legacy.beginStroke({5,5},legacy.getLayer(1).canvas);
    for(int i=6;i<55;++i) {
        glm::vec2 p{i, 20+10*std::sin(i*0.3f)};
        legacy.addStrokePoint(p); modern.pointerDragged(p);
        if(i%5==0) { legacy.drawStroke(brush); modern.drawCanvas(true); }
    }
    legacy.drawStroke(brush); legacy.commitStroke(legacy.getLayer(1).canvas); modern.pointerReleased();
    ofFloatPixels expected; legacy.getLayer(1).canvas.readToPixels(expected);
    equal(pixels(modern,1),expected,"legacy/new stroke pixel equivalence");

    paintcoreui::PaintEditor editor(config);
    editor.setup(); editor.draw();
    editor.mousePressed(10,10,0); editor.mouseDragged(30,30,0); editor.mouseReleased(30,30,0);
    require(editor.paint.canUndo(),"standard UI input forwarding");

    // The identity and random angle of a stamp must survive redraws and checkpoints.
    paintcore::PaintSession stamps(config); stamps.setup();
    auto randomBrush = stamps.getBrushSettings(); randomBrush.randomAngleDegrees = 180;
    stamps.setBrushSettings(randomBrush);
    std::map<std::uint64_t, paintcore::StampInfo> seen;
    stamps.engine.brush.setDetailedStamp([&seen](const paintcore::StampInfo& info) {
        const auto old = seen.find(info.index);
        if (old != seen.end()) {
            require(old->second.position == info.position,"stable stamp position");
            require(old->second.angleDegrees == info.angleDegrees,"stable random rotation");
        }
        seen[info.index] = info;
        ofDrawCircle(info.position.x, info.position.y, info.radius);
    });
    stamps.pointerPressed({2,2});
    for (int i=3; i<60; ++i) { stamps.pointerDragged({i,30}); stamps.drawCanvas(true); }
    stamps.pointerReleased();
    require(seen.size()>10,"stamp indices across checkpoints");
    paintcore::CanvasView view; view.origin={10,20}; view.scale=2;
    require(glm::distance(view.toCanvas({30,40}),glm::vec2(10,10))<0.001f,"view input transform");
}
class TestApp : public ofBaseApp {
    void setup() override {
        ofLogToConsole();
        ofDirectory::createDirectory(testPath(""),true,true);
        int code=0;
        try { tests(); ofBufferToFile(testPath("paintcore-test-result.txt"),ofBuffer("PASS\n",5)); }
        catch (const std::exception& e) { code=1; ofBuffer result; result.set(std::string("FAIL: ")+e.what()); ofBufferToFile(testPath("paintcore-test-result.txt"),result); }
        ofExit(code);
    }
};
}
int main() {
    ofGLWindowSettings settings; settings.setSize(64,64);
    auto window=ofCreateWindow(settings);
    ofRunApp(window,std::make_shared<TestApp>());
    ofRunMainLoop();
}
