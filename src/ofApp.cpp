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
bool drawingFlag = false;
bool pickerFlag = false;

//--------------------------------------------------------------
void ofApp::setup() {
	ofLogToConsole(); // ログの出力先をコンソールに設定

	ofSetFrameRate(60);

	ui.undo.addListener(&paintTool, &PaintTool::undo);
	ui.redo.addListener(&paintTool, &PaintTool::redo);
	ui.deleteAll.addListener(this, &ofApp::beforeDeleteAll);

	const int initialLayerId = paintTool.addLayer(LayerType::Normal, false);

	ofLogNotice("Layers")
		<< "Created Layer" << initialLayerId
		<< ", count=" << paintTool.layers.size()
		<< ", selected=" << paintTool.selectedLayerId;

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

	if (drawingFlag) {
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

			// Reactの表示から消える直前の行が押される場合もあるため、
			// 現在も存在するIDか確認してから選択する
			for (const auto& layer : paintTool.layers) {
				if (layer->id == layerId) {
					paintTool.selectedLayerId = layerId;
					break;
				}
			}
		} else if (command == 7) {
			paintTool.deleteLayer(getPaintCommandLayerId());
		} else if (command == 8) {
			const int layerId = getPaintCommandLayerId();
			const int targetLayerId = getPaintCommandTargetLayerId();

			int destinationIndex = -1;

			for (int i = 0; i < static_cast<int>(paintTool.layers.size()); ++i) {
				if (paintTool.layers[i]->id == targetLayerId) {
					destinationIndex = i;
					break;
				}
			}

			if (destinationIndex != -1) {
				paintTool.moveLayer(layerId, destinationIndex);
			}
		} else if (command >= 9 && command <= 11) {
			const int layerId = getPaintCommandLayerId();
			const float value = static_cast<float>(getPaintCommandValue());

			for (auto& layer : paintTool.layers) {
				if (layer->id != layerId) {
					continue;
				}

				if (layer->filter->getType() != LayerType::Glow) {
					break;
				}

				auto * glow = static_cast<GlowFilter *>(layer->filter.get());

				float sigma = glow->getSigma();
				float strength = glow->getStrength();
				float period = glow->getPeriod();

				if (command == 9) {
					sigma = value;
				} else if (command == 10){
					strength = value;
				} else {
					period = value;
				}

				glow->setParameters(sigma, strength, period);
				break;
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
	ofBackground(ofColor::fromHex(getBackgroundColor()));
	const bool isPen = getPenMode() != 0;

	if (isPen) {
		paintTool.strokeSize = getPenSize();
	} else {
		paintTool.strokeSize = getEraserSize();
	}
	paintTool.color = ofColor::fromHex(getPaintColor(), getPenAlpha() * 255);
	if (drawingFlag) {
		paintTool.drawLine(isPen, getEraserAlpha() * 255);
	}
#else
	ofBackground(20);
	const bool isPen = ui.penSwitch;

	if (isPen) {
		paintTool.strokeSize = ui.penSize;
	} else {
		paintTool.strokeSize = ui.eraserSize;
	}
	paintTool.color = ofColor(paintTool.color.r, paintTool.color.g, paintTool.color.b, ui.penAlpha * 255);
	if (drawingFlag) {
		paintTool.drawLine(isPen, ui.eraserAlpha * 255);
	}

	ofSetColor(100);
	ofDrawRectangle(mainWidth, 256, 256, ofGetHeight() - 256);

	ui.placeUi(isPen);
	ui.updatePicker(mainWidth);
	if (pickerFlag) {
		ui.picking(ofGetMouseX(), ofGetMouseY(), mainWidth);
		paintTool.color = ui.pickedColor;
	}
#endif // __EMSCRIPTEN__

	paintTool.drawCanvas(drawingFlag);

#ifndef __EMSCRIPTEN__
	ui.drawLayerPanel(
		paintTool.layers,
		paintTool.selectedLayerId);
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
	if (drawingFlag) {
		paintTool.points.push_back(glm::vec2(x, y));

		//"When the coordinates are too far apart, interpolate the values in between"
		if (paintTool.points.size() > 1) {
			const glm::vec2 currentPoint = paintTool.points.back();
			const glm::vec2 previousPoint = paintTool.points[paintTool.points.size() - 2];

			vector<glm::vec2> newPoints; //Points for interpolation
			const float pointsDist = glm::distance(currentPoint, previousPoint);
			float idealDist = 3;
			if (pointsDist > idealDist) {
				float NumberOfPoint = pointsDist / idealDist;
				float span = 1 / NumberOfPoint;

				for (float i = span; i < 1; i += span) {
					glm::vec2 point = glm::mix(currentPoint, previousPoint, i);
					float lastDist = glm::distance(point, previousPoint); //`lastDist` is the distance between the last point to be interpolated and the last point before interpolation.
					//If `lastDist` is too small, the last point is unnecessary.
					if (lastDist > 0.7) {
						newPoints.push_back(point);
					}
				}

				paintTool.points.insert(
					paintTool.points.end() - 1,
					newPoints.begin(),
					newPoints.end());
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
	if (button != OF_MOUSE_BUTTON_LEFT) {
		return;
	}

#ifndef __EMSCRIPTEN__
	if (!drawingFlag) {
		const LayerUiAction action = ui.pressLayerPanel(x, y, paintTool.layers);

		switch (action.type) {
		case LayerUiActionType::Select:
			paintTool.selectedLayerId = action.layerId;
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
			ui.scrollLayerPanel(0.0f, static_cast<int>(paintTool.layers.size()));
			return;

		case LayerUiActionType::Move:
		case LayerUiActionType::None:
			break;
		}
	}
#endif

	if (0 <= x && x < mainWidth && 0 <= y && y < mainHeight) {

		paintTool.beginStroke();

		drawingFlag = true;
		paintTool.points.push_back(glm::vec2(x, y));
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
		static_cast<int>(paintTool.layers.size()));

	if (action.type == LayerUiActionType::Move) {
		paintTool.moveLayer(
			action.layerId,
			action.destinationIndex);
	}
#endif

	if (drawingFlag) {
#ifdef __EMSCRIPTEN__
		const bool isPen = getPenMode() != 0;
		if (isPen) {
			paintTool.strokeSize = getPenSize();
		} else {
			paintTool.strokeSize = getEraserSize();
		}
		paintTool.color = ofColor::fromHex(getPaintColor(), getPenAlpha() * 255);
		paintTool.drawLine(isPen, getEraserAlpha() * 255);
#else
		const bool isPen = ui.penSwitch;
		if (isPen) {
			paintTool.strokeSize = ui.penSize;
		} else {
			paintTool.strokeSize = ui.eraserSize;
		}
		paintTool.color = ofColor(paintTool.color.r, paintTool.color.g, paintTool.color.b, ui.penAlpha * 255);
		paintTool.drawLine(isPen, ui.eraserAlpha * 255);
#endif

		paintTool.commitStroke();
	}

	drawingFlag = false;
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
	if (drawingFlag) {
		return;
	}

	paintTool.deleteAll();
}

//--------------------------------------------------------------
void ofApp::mouseScrolled(
	int x, int y, float scrollX, float scrollY) {
#ifndef __EMSCRIPTEN__
	if (drawingFlag) {
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
			static_cast<int>(paintTool.layers.size()));
	}
#endif
}

#ifdef __EMSCRIPTEN__
// JSON形式の変数に選択中のIDとそれに対応するレイヤー情報を入れて、文字列に変換して
// それを引数としてpublishLayerStateを呼ぶ関数
// 要は選択中のレイヤーの情報をReactに教えるためのもの
void ofApp::syncLayerStateToWeb() {
	ofJson state;
	state["selectedLayerId"] = paintTool.selectedLayerId;
	state["layers"] = ofJson::array();

	for (const auto& layer : paintTool.layers) {
		ofJson item;

		item["id"] = layer->id;
		item["name"] = layer->name;
		item["type"] = layer->filter->getType() == LayerType::Normal
			? "Normal"
			: "Glow";

		item["glow"] = nullptr;

		if (layer->filter->getType() == LayerType::Glow) {
			const auto * glow = static_cast<const GlowFilter *>(layer->filter.get());

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

	glowControlsLayerId = paintTool.selectedLayerId;

	syncingGlowControls = false;
}

void ofApp::onGlowParameterChanged(float&) {
	if (syncingGlowControls || drawingFlag) {
		return;
	}

	if (glowControlsLayerId != paintTool.selectedLayerId) {
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
}
#endif // __EMSCRIPTEN__
