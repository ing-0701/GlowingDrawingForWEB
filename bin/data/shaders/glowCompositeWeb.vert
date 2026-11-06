#version 300 es

precision highp float;

uniform mat4 modelViewProjectionMatrix;

in vec4 position;
in vec2 texcoord;

out vec2 textCoord;

void main(){
	gl_Position = modelViewProjectionMatrix * position;
	textCoord = texcoord;
}

// oFでシェーダーを有効化し、画像を描く命令を出す。位置とテクスチャが渡される。
// oFからの命令は、ofShader.begin後のFbo.drawなど。
// ftransformによってoFの座標系からシェーダー用の座標系に変換された位置情報がgl_Positionに入る。
// textCoordにoFの命令をもとにしたテクスチャ座標が与えられる(この時点では4つ)。
// 頂点シェーダーからフラグメントシェーダーにtextCoordを渡す際、varyingという宣言に従い、GPUがtextCoordを補完する。
