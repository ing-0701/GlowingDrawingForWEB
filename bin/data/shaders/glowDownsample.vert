#version 120

varying vec2 textCoord;

void main() {
	gl_Position = ftransform();    //描画する図形の頂点の位置(今回は4つ)
	textCoord = gl_MultiTexCoord0.xy;    //元画像のどの部分か
}


// oFでシェーダーを有効化し、画像を描く命令を出す。位置とテクスチャが渡される。
// oFからの命令は、ofShader.begin後のFbo.drawなど。
// ftransformによってoFの座標系からシェーダー用の座標系に変換された位置情報がgl_Positionに入る。
// textCoordにoFの命令をもとにしたテクスチャ座標が与えられる(この時点では4つ)。
// 頂点シェーダーからフラグメントシェーダーにtextCoordを渡す際、varyingという宣言に従い、GPUがtextCoordを補完する。
