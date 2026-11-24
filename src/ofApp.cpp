#include "ofApp.h"

#ifdef __EMSCRIPTEN__ //以下#endifまで、emscriptenでビルドするときのみ有効
	#include <emscripten.h>

EM_JS(int, getBackgroundColor, (), {
	return window['paintBackgroundColor'] ?? 0xffffff;
})

EM_JS(int, getPaintColor, (), {
	return window['paintColor'] ?? 0;
})

EM_JS(double, getPenSize, (), {
	return window['paintPenSize'] ?? 0.1;
})

EM_JS(double, getPenAlpha, (), {
	return window['paintPenAlpha'] ?? 1;
})

EM_JS(double, getEraserSize, (), {
	return window['paintEraserSize'] ?? 0.1;
})

EM_JS(double, getEraserAlpha, (), {
	return window['paintEraserAlpha'] ?? 1;
})

EM_JS(int, getPenMode, (), {
	const value = window['paintPenMode'];

	if (typeof value === 'boolean') {
		return value ? 1 : 0;
	}

	return 1;
})

EM_JS(int, takePaintCommand, (), {
	const commands = window['paintCommands'];

	window['paintCommandLayerId'] = 0;
	window['paintCommandTargetLayerId'] = 0;
	window['paintCommandValue'] = 0;

	if (!commands || commands.length === 0) {
		return 0;
	}

	const command = commands.shift();

	if (command === 'undo') return 1;
	if (command === 'redo') return 2;
	if (command === 'deleteAll') return 3;
	if (command === 'addNormal') return 4;
	if (command === 'addGlow') return 5;

	// commandがnullでなく、object型の場合
	if (command && typeof command === 'object') {
		// 整数か？0じゃないか？
		if (!Number.isInteger(command.layerId) || command.layerId <= 0) {
			return 0;
		}

		window['paintCommandLayerId'] = command.layerId;

		if (command.type === 'setGlowParameter') {
			// 値が有限か判定
			if (!Number.isFinite(command.value)) {
				return 0;
			}

			window['paintCommandValue'] = command.value;

			if (command.parameter === 'sigma') return 9;
			if (command.parameter === 'strength') return 10;
			if (command.parameter === 'period') return 11;

			return 0;
		}

		if (command.type === 'selectLayer') return 6;
		if (command.type === 'deleteLayer') return 7;

		if (command.type === 'moveLayer') {
			if (!Number.isInteger(command.targetLayerId)
				|| command.targetLayerId <= 0) {
				return 0;
			}

			window['paintCommandTargetLayerId'] = command.targetLayerId;

			return 8;
		}
	}

	return 0;
})

EM_JS(double, getPaintCommandValue, (), {
	return window['paintCommandValue'];
})

EM_JS(int, getPaintCommandLayerId, (), {
	return window['paintCommandLayerId'];
})

EM_JS(int, getPaintCommandTargetLayerId, (), {
	return window['paintCommandTargetLayerId'];
})

// UTF8としてC++からWASMを通して受け取った文字列をJSONに直してReactに渡す関数
EM_JS(void, publishLayerState, (const char * jsonText), {
	const text = UTF8ToString(jsonText);

	// 内容が変わっていなければ、前回のデータをそのまま使う
	if (window['paintLayerStateText'] === text) {
		return;
	}

	window['paintLayerState'] = JSON.parse(text);
	window['paintLayerStateText'] = text;
})
#endif

bool modelingFlag = false;
bool pickerFlag = false;

//--------------------------------------------------------------
void ofApp::setup() {
	ofLogToConsole(); // ログの出力先をコンソールに設定

	ofSetFrameRate(60);

	ui.undo.addListener(&paintTool, &PaintTool::undo);
	ui.redo.addListener(&paintTool, &PaintTool::redo);
	ui.deleteAll.addListener(this, &ofApp::beforeDeleteAll);

	paintTool.setup();
	const int initialLayerId = paintTool.getSelectedLayerId();

	ofLogNotice("Layers")
		<< "Created Layer" << initialLayerId
		<< ", count=" << paintTool.engine.getLayers().size()
		<< ", selected=" << paintTool.getSelectedLayerId();

#ifndef __EMSCRIPTEN__
	ui.glowSigma.addListener(this, &ofApp::onGlowParameterChanged);
	ui.glowStrength.addListener(this, &ofApp::onGlowParameterChanged);
	ui.glowPeriod.addListener(this, &ofApp::onGlowParameterChanged);

	syncGlowControls();
#endif // !__EMSCRIPTEN__
}

//--------------------------------------------------------------
void ofApp::update() {
#ifdef __EMSCRIPTEN__
	// 初回のupdateで、画面更新ごとに実行する設定へ変更する
	static bool loopTimingConfigured = false;

	if (!loopTimingConfigured) {
		const int result = emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);

		loopTimingConfigured = (result == 0);
	}
	// 上の内容ちゃんと理解してないからいつか確認して

	if (paintTool.isDrawing()) {
		return;
	}

	int command = takePaintCommand();

	while (command != 0) {
		if (command == 1) {
			paintTool.undo();
		} else if (command == 2) {
			paintTool.redo();
		} else if (command == 3) {
			beforeDeleteAll();
		} else if (command == 4) {
			paintTool.addLayer(LayerType::Normal);
		} else if (command == 5) {
			paintTool.addLayer(LayerType::Glow);
		} else if (command == 6) {
			const int layerId = getPaintCommandLayerId();

			// findLayerIndexで-1以外が返ってきたならちゃんと存在するID
			if (paintTool.findLayerIndex(layerId) >= 0) {
				paintTool.selectLayer(layerId);
			}
		} else if (command == 7) {
			paintTool.deleteLayer(getPaintCommandLayerId());
		} else if (command == 8) {
			const int layerId = getPaintCommandLayerId();
			const int targetLayerId = getPaintCommandTargetLayerId();

			const int destinationIndex = paintTool.findLayerIndex(targetLayerId);

			if (destinationIndex != -1) {
				paintTool.moveLayer(layerId, destinationIndex);
			}
		} else if (command >= 9 && command <= 11) {
			const int layerId = getPaintCommandLayerId();
			const float value = static_cast<float>(getPaintCommandValue());

			if (GlowFilter* glow = paintTool.getGlowFilter(layerId)) {
				float sigma = glow->getSigma();
				float strength = glow->getStrength();
				float period = glow->getPeriod();

				if (command == 9) {
					sigma = value;
				} else if (command == 10) {
					strength = value;
				} else {
					period = value;
				}

				glow->setParameters(sigma, strength, period);
                paintTool.markDocumentChanged();
			}
		}

		command = takePaintCommand();
	}

	syncLayerStateToWeb();
#else
	syncGlowControls();
#endif // __EMSCRIPTEN__
}

//--------------------------------------------------------------
void ofApp::draw() {
#ifdef __EMSCRIPTEN__
	paintTool.setBackground(ofColor::fromHex(getBackgroundColor()));
    ofBackground(paintTool.getBackground());
#else
	paintTool.setBackground(ofColor(20));
    ofBackground(paintTool.getBackground());
	const bool isPen = ui.penSwitch;

	ofSetColor(100);
	ofDrawRectangle(mainWidth, 256, 256, ofGetHeight() - 256);

	ui.placeUi(isPen);
	ui.updatePicker(mainWidth);
	if (pickerFlag) {
		ui.picking(ofGetMouseX(), ofGetMouseY(), mainWidth);
		brushColor = ui.pickedColor;
	}
#endif // __EMSCRIPTEN__

	paintTool.drawCanvas(paintTool.isDrawing());

#ifndef __EMSCRIPTEN__
	ui.drawLayerPanel(
		paintTool.engine.getLayers(),
		paintTool.getSelectedLayerId(),
		[this](int layerId) {
			return paintTool.getFilter(layerId).getType();
		}
	);
#endif
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
	if (paintTool.isDrawing()) {
		paintTool.pointerDragged(glm::vec2(x, y));
	}
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
	if (button != OF_MOUSE_BUTTON_LEFT) {
		return;
	}

#ifndef __EMSCRIPTEN__
	if (!paintTool.isDrawing()) {
		const LayerUiAction action = ui.pressLayerPanel(x, y, paintTool.engine.getLayers());

		switch (action.type) {
		case LayerUiActionType::Select:
			paintTool.selectLayer(action.layerId);
			return;

		case LayerUiActionType::AddNormal:
			paintTool.addLayer(LayerType::Normal);
			return;

		case LayerUiActionType::AddGlow:
			paintTool.addLayer(LayerType::Glow);
			return;

		case LayerUiActionType::Delete:
			paintTool.deleteLayer(action.layerId);

			// 削除後の枚数に合わせてスクロール位置を補正する
			ui.scrollLayerPanel(0.0f, static_cast<int>(paintTool.engine.getLayers().size()));
			return;

		case LayerUiActionType::Move:
		case LayerUiActionType::None:
			break;
		}
	}
#endif

	if (0 <= x && x < mainWidth && 0 <= y && y < mainHeight) {

        paintTool.setBrushSettings(readBrushSettings());
        paintTool.pointerPressed(glm::vec2(x, y));
	}

#ifndef __EMSCRIPTEN__
	if (mainWidth <= x && x < ofGetWidth() && 0 <= y && y < 256) {
		pickerFlag = true;
	}
#endif // !__EMSCRIPTEN__
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	if (button != OF_MOUSE_BUTTON_LEFT) {
		return;
	}

#ifndef __EMSCRIPTEN__
	const LayerUiAction action = ui.releaseLayerPanel(
		x,
		y,
		static_cast<int>(paintTool.engine.getLayers().size()));

	if (action.type == LayerUiActionType::Move) {
		paintTool.moveLayer(
			action.layerId,
			action.destinationIndex);
	}
#endif

    paintTool.pointerReleased();

	pickerFlag = false;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
}

//--------------------------------------------------------------
void ofApp::exit() {
	ui.undo.removeListener(&paintTool, &PaintTool::undo);
	ui.redo.removeListener(&paintTool, &PaintTool::redo);
	ui.deleteAll.removeListener(this, &ofApp::beforeDeleteAll);
#ifndef __EMSCRIPTEN__
	ui.glowSigma.removeListener(
		this, &ofApp::onGlowParameterChanged);
	ui.glowStrength.removeListener(
		this, &ofApp::onGlowParameterChanged);
	ui.glowPeriod.removeListener(
		this, &ofApp::onGlowParameterChanged);
#endif
}

//--------------------------------------------------------------
void ofApp::beforeDeleteAll() {
	if (paintTool.isDrawing()) {
		return;
	}

	paintTool.deleteAll();
}

//--------------------------------------------------------------
void ofApp::mouseScrolled(
	int x, int y, float scrollX, float scrollY) {
#ifndef __EMSCRIPTEN__
	if (paintTool.isDrawing()) {
		return;
	}

	// レイヤー一覧が表示されている領域
	const ofRectangle listArea(
		mainWidth + 104, 386,
		148, 46 * 4);

	// レイヤー一覧の上でスクロールした場合だけ処理する
	if (listArea.inside(x, y)) {
		ui.scrollLayerPanel(
			scrollY,
			static_cast<int>(paintTool.engine.getLayers().size()));
	}
#endif
}

#ifdef __EMSCRIPTEN__
// JSON形式の変数に選択中のIDとそれに対応するレイヤー情報を入れて、文字列に変換して
// それを引数としてpublishLayerStateを呼ぶ関数
// 要は選択中のレイヤーの情報をReactに教えるためのもの
void ofApp::syncLayerStateToWeb() {
	ofJson state;
	state["selectedLayerId"] = paintTool.getSelectedLayerId();
	state["layers"] = ofJson::array();

	for (const auto & layer : paintTool.engine.getLayers()) {
		ofJson item;

		const GlowFilter * glow = paintTool.getGlowFilter(layer->id);

		item["id"] = layer->id;
		item["name"] = layer->name;
		item["type"] = glow ? "Glow" : "Normal";
		item["glow"] = nullptr;

		if (glow) {
			item["glow"] = {
				{ "sigma", glow->getSigma() },
				{ "strength", glow->getStrength() },
				{ "period", glow->getPeriod() }
			};
		}

		state["layers"].push_back(item);
	}

	const std::string text = state.dump();

	publishLayerState(text.c_str());
}
#else
void ofApp::syncGlowControls() {
	GlowFilter * glow = paintTool.getSelectedGlowFilter();
	const bool enabled = glow != nullptr;

	// 選択中のレイヤーのモードを判定
	// GlowModeなのにスライダーが使えない状態だったなら使えるように
	// NormalModeなのに使える状態だったなら使えないようにする
	if (enabled != ui.glowControlsEnabled) {
		if (enabled) {
			ui.glowSigma.registerMouseEvents();
			ui.glowStrength.registerMouseEvents();
			ui.glowPeriod.registerMouseEvents();

		} else {
			ui.glowSigma.unregisterMouseEvents();
			ui.glowStrength.unregisterMouseEvents();
			ui.glowPeriod.unregisterMouseEvents();
		}
	}

	ui.glowControlsEnabled = enabled;

	if (!glow) {
		glowControlsLayerId = 0;
		return;
	}

	// 以下はユーザー操作ではなく、保存済みの値の表示
	syncingGlowControls = true;

	ui.glowSigma = glow->getSigma();
	ui.glowStrength = glow->getStrength();
	ui.glowPeriod = glow->getPeriod();

	glowControlsLayerId = paintTool.getSelectedLayerId();

	syncingGlowControls = false;
}

void ofApp::onGlowParameterChanged(float &) {
	if (syncingGlowControls || paintTool.isDrawing()) {
		return;
	}

	if (glowControlsLayerId != paintTool.getSelectedLayerId()) {
		return;
	}

	GlowFilter * glow = paintTool.getSelectedGlowFilter();

	if (!glow) {
		return;
	}

	glow->setParameters(
		ui.glowSigma,
		ui.glowStrength,
		ui.glowPeriod);
    paintTool.markDocumentChanged();
}
#endif // __EMSCRIPTEN__

paintcore::BrushSettings ofApp::readBrushSettings() {
#ifdef __EMSCRIPTEN__
    const bool pen = getPenMode() != 0;
    return { static_cast<float>(pen ? getPenSize() : getEraserSize()),
        ofColor::fromHex(getPaintColor(), getPenAlpha() * 255), pen,
        static_cast<float>(getEraserAlpha() * 255) };
#else
    const bool pen = ui.penSwitch;
    ofColor color = brushColor;
    color.a = static_cast<unsigned char>(ui.penAlpha * 255);
    return { pen ? static_cast<float>(ui.penSize) : static_cast<float>(ui.eraserSize),
        color, pen, ui.eraserAlpha * 255 };
#endif
}
