#include "ofMain.h"
#include "ofApp.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif // __EMSCRIPTEN__


//========================================================================
int main( ){

	//Use ofGLFWWindowSettings for more options like multi-monitor fullscreen
	ofGLWindowSettings settings;
	settings.setSize(800+256, 600);
	settings.windowMode = OF_WINDOW; //can also be OF_FULLSCREEN

	auto window = ofCreateWindow(settings);

#ifdef __EMSCRIPTEN__
	const auto context = emscripten_webgl_get_current_context();

	const bool canRenderFloat = emscripten_webgl_enable_extension(context, "EXT_color_buffer_float");
	const bool canBlendFloat = emscripten_webgl_enable_extension(context, "EXT_float_blend");
	const bool canFilterFloat = emscripten_webgl_enable_extension(context, "OES_texture_float_linear");

	if (!canRenderFloat || !canBlendFloat || !canFilterFloat) {
		ofLogError("main")
			<< "Required RGBA32F support is unavailable: "
			<< "render=" << canRenderFloat
			<< ", blend=" << canBlendFloat
			<< ", linearFilter=" << canFilterFloat;

		return 1;
	}
#endif // __EMSCRIPTEN__


	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();

}
