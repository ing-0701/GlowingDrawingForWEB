# PaintCoreとGlowingDrawingの構成

今回の変更で、標準的なペイント操作をつなぐ処理を `PaintSession` にまとめました。各アプリで、レイヤーIDを発行し、描き始めたレイヤーを記録し、画像を履歴へコピーし、削除したフィルターを保管する処理を毎回書く必要がなくなりました。

GlowingDrawingは、この共通機能にNormalFilter／GlowFilter、既存のWindows UI、Reactとの通信、動画保存を接続します。発光の計算式やシェーダー、Reactのレイアウト、動画のアップロードとQRコード発行は、引き続きGlowingDrawing側です。

## 全体のつながり

```text
WindowsのUi           ReactのofViewer.tsx
      │                     │ iframe内のwindow変数・コマンド
      └──────── ofApp ────────┘
                  │ UIの値をBrushSettingsへ変換
                  ▼
              PaintTool
                  │ Normal／Glowの拡張オブジェクトを選ぶ
                  ▼ 継承
          paintcore::PaintSession
            ├─ PaintEngine ─ StrokeSampler ＋ Brush
            ├─ Layerの配列、選択ID、次のID
            ├─ HistoryManager ＋ SessionHistoryEntry
            └─ CanvasRenderer ─ LayerExtensionのdraw
                                      ▲
                         NormalFilter／GlowFilter
```

ここで「継承」とは、PaintToolがPaintSessionの公開機能をそのまま持つ、という意味です。`paintTool.undo()` を呼んだ先の標準処理は、現在はPaintSessionにあります。

## ファイルごとの役割

以下のコアファイルはすべて `addons/ofxPaintCore/src` にあります。

| ファイル | 責任 |
|---|---|
| PaintSettings.h / .cpp | 初期設定とFBO設定の生成 |
| PaintSession.h / .cpp | 標準操作の入口、描画状態、選択、ID、履歴との接続 |
| PaintEngine.h / .cpp | レイヤー画像、ストローク用画像、途中保存、ブラシ描画 |
| StrokeSampler.h / .cpp | 入力された点からベジェ曲線をたどり、スタンプ位置を決める |
| Brush.h / .cpp | 各スタンプ位置で円や画像を描く。ペン／消しゴムの合成も担当 |
| PaintLayer.h / .cpp | ID、名前、キャンバス画像など1枚分のデータ |
| CanvasRenderer.h / .cpp | レイヤーの描画順とプレビュー画像の選択 |
| LayerExtension.h | 各アプリが独自表示・独自データを追加する接続口 |
| HistoryAction.h | UndoとRedoを実装するための抽象クラス |
| HistoryManager.h | 履歴の順番、上限、Undo/Redoの位置を管理 |
| CanvasView.h | 表示位置・倍率と入力座標の変換 |
| ColorPicker.h / .cpp | 任意で使えるoF用の色選択部品 |
| PaintEditor.h | 基本機能を接続済みのofxGui UI |

`PaintEngine` は画像を扱う比較的低い層、`PaintSession` はそれらをペイントソフトの操作にまとめる層です。たとえばEngineの `commitStroke(canvas)` は画像をコピーします。一方Sessionの `pointerReleased()` は、必要なプレビュー更新、履歴保存、画像コピー、表示更新通知、描画状態の解除まで行います。

新しいアプリを作るときは、まずPaintSessionまたはPaintEditorを使います。個別の描画処理を特殊化する必要が出たときにEngine以下へ降りていく構成です。

## 初期化で何が準備されるか

設定を省略すると、800×600、RGBA32F、履歴20件になります。`PaintSession` のコンストラクタは設定を受け取り、OpenGLを使う処理は `setup()` まで待ちます。

`setup()` はLayer1とストローク用画像を準備し、履歴画像と交換用画像を確保し、最初の拡張オブジェクトを対応付けます。その後、選択IDを1、次に発行するIDを2に設定します。

GlowingDrawingの `PaintTool::setup()` は、この入口にNormalFilterを渡します。共通機能だけのアプリなら、引数なしの `setup()` で通常描画が使えます。setupは1回だけです。作品を空にして最初から始める操作は `newDocument()` に分けています。

## 一本の線ができるまで

### 押した瞬間

`ofApp::mousePressed()` は、UI上のクリックかキャンバス上のクリックかを判別します。キャンバスなら `readBrushSettings()` で現在の色・太さ・不透明度・ペンか消しゴムかを読み、PaintSessionへ渡します。

Webでは `readBrushSettings()` がiframe内の `window.paintColor`、`window.paintPenSize` などを読みます。WindowsではUiのスライダーと `brushColor` を読みます。どちらも最後は同じ `BrushSettings` になります。

`pointerPressed(position)` は、キャンバス内か、すでに描画中でないかを確認します。そのうえで選択中レイヤーIDをストロークの対象として記録し、その時点のブラシ設定を `activeBrush` にコピーします。

設定のコピーは、この線が途中で別のレイヤーや別のブラシ設定へ変わらないためです。次の線を描き始めれば、新しいスライダー値が使われます。

Engineは確定済みレイヤーを `strokeCheckpointFbo` にコピーし、最初のスタンプ位置を登録します。この段階で確定済み画像は変更しません。

### ドラッグ中

`pointerDragged(position)` からStrokeSamplerに位置が渡ります。StrokeSamplerは従来の二次ベジェ曲線を使い、原則3px間隔でブラシを置く位置を生成します。

表示が必要になると、PaintSessionがEngineの `drawStroke(activeBrush)` を呼びます。Engineは途中保存画像を `preFbo2D` にコピーし、まだ途中保存していないスタンプをその上へ描きます。この `preFbo2D` が描画中のプレビューです。

未保存点が10点以上になれば、プレビューを途中保存画像へ移します。後続の描画では古い全点を描き直さずに済みます。この処理は履歴を増やしません。長い一本の線も、Undo上は一本です。

`previewDirty` は入力が増えて再描画が必要な状態を表します。入力が変わらないフレームでは、同じプレビューを作り直しません。Glowの点滅はフィルターのdrawが時間に応じて表示するため、そのまま動きます。

### 放した瞬間

`ofApp::mouseReleased()` は `pointerReleased()` を呼びます。座標を渡していないため、放した位置に新しい点を追加しません。

PaintSessionは、必要なら最後のドラッグまでをプレビューへ反映し、変更前のレイヤー画像を履歴の作業枠へ保存します。その後プレビューをレイヤーへコピーし、履歴を確定して、拡張へ `markDirty()` を送り、ストローク対象IDを0へ戻します。

この順番により、描き始めとリリースの間にdrawが呼ばれなかった場合でも線を確定できます。また、履歴へ保存されるのは線を描き込む前の画像です。

## レイヤーと発光処理の分離

`Layer` にフィルターのメンバはありません。PaintSessionの `extensions` が「ID → LayerExtensionのオブジェクト」を保持します。GlowingDrawingのLayerFilterは、このLayerExtensionを継承します。さらにNormalFilterとGlowFilterがLayerFilterを継承します。

共通コアはNormalとGlowという種類分けを知る必要がありません。画像を変更したら `markDirty()`、表示するときは `draw(source, time)` と呼びます。その先の処理は実際のオブジェクトによって変わります。

NormalFilterは画像を通常合成します。GlowFilterは既存の縮小・ガウシアン・加算合成・点滅を行います。発光の式、設定範囲、シェーダーは今回変更していません。

CanvasRendererは配列を末尾から先頭へたどります。したがって一覧の先頭ほど手前に表示されます。描画中の対象レイヤーだけは確定画像の代わりにプレビューを渡します。ほかのレイヤーには、それぞれの確定画像を渡します。

PaintCoreのLayerは位置・表示状態・不透明度を持ちません。これらが必要なアプリでは、そのアプリが表示と入力座標の変換を管理します。通常のズーム・パンには `CanvasView` を利用できます。

## レイヤー操作と選択

`addLayer()` はID発行、名前、画像確保、選択中レイヤーの上への挿入、新規レイヤーの選択、履歴登録をまとめます。PaintToolはNormalFilterかGlowFilterを作ってこの関数へ渡すだけになりました。

`selectLayer(id)` は描画中でなく、IDが実在する場合に選択を更新します。アプリは `selectedLayerId` を直接書き換えません。Reactの古いコマンドなどで存在しないIDが渡っても選択状態を壊さないためです。

`deleteLayer(id)` は最後の1枚を残します。選択中レイヤーを消す場合は、一覧でその下のレイヤーを選び、下がなければ上を選びます。別のレイヤーを消す場合は選択を維持します。

`deleteAll()` は選択中の画像だけを透明な黒で消します。NormalとGlowのどちらでも同じ標準処理です。Glowの場合はその後の通知により、古い発光画像が再利用されないようになります。

## UndoとRedoの中身

HistoryManagerは「どの操作が何を意味するか」を判断しません。履歴の順番を管理し、その履歴のundoまたはredoを呼びます。

SessionHistoryEntryには標準の種類があります。

| 種類 | 記憶する内容 | Undo/Redoの処理 |
|---|---|---|
| CanvasEdit | 操作対象IDと変更前の画像 | 現在画像と履歴画像を交換 |
| LayerAdded | ID、位置、前後の選択IDと次のID | Undoで取り外し、Redoで再挿入 |
| LayerDeleted | 取り外したレイヤーと拡張、位置、選択 | Undoで再挿入、Redoで取り外し |
| Custom | HistoryActionの派生オブジェクト | そのオブジェクトのundo/redoを呼ぶ |

画像交換では、交換用FBOを経由して「現在 → 交換用」「履歴 → 現在」「交換用 → 履歴」とコピーします。Undoした後には、Redoするための画像が同じ履歴枠に入ります。描画も透明消しゴムも全消しもこの方式です。

レイヤー削除では、unique_ptrを履歴へ移します。オブジェクトそのものは破棄されません。Undoでは同じLayerと同じ拡張オブジェクトを配列と対応表へ戻すので、画素だけでなく発光パラメータも維持できます。履歴が上限を超えてその操作を戻せなくなれば、履歴だけが所有するオブジェクトは解放されます。

レイヤー追加のUndoでは次のIDも戻します。新しいレイヤーを追加し直したときに、使われなかった番号として再利用できます。

`prepare()` は別の作業枠を用意し、`finish()` が成功した操作をリングバッファへ確定します。Redoの破棄もfinishで行います。準備しただけでRedoを失うことを避ける構成です。そのため履歴件数分とは別に、作業用画像が1枚あります。

レイヤーの追加・削除と描画内容は履歴に記録されます。並べ替えは `moveLayer()` が配列順を変更しますが、Undo/Redoには入りません。Glowスライダーの値もGlowingDrawing側の設定であり、Undo対象ではありません。

## Reactとの通信

React側のソースは今回変更していません。`paintColor`、`paintPenSize`、`paintPenAlpha`、`paintEraserSize`、`paintEraserAlpha`、`paintPenMode`、`paintBackgroundColor` を使う経路を残しています。

UI操作は `paintCommands` に入り、ofAppのupdateが順に取り出します。Undo/Redo、全消し、レイヤー操作は現在のPaintToolから共通処理へ届きます。Glowパラメータ変更はGlowingDrawing側でGlowFilterを取得して変更します。

描画中はコマンドを取り出さずに待たせる従来の仕様です。コア側にも描画中の操作ガードがあり、アプリの判断だけに依存しません。

戻りの `paintLayerState` は、引き続き `selectedLayerId` と `layers` を含みます。各要素の `id`、`name`、`type`、`glow` と、Glow内の `sigma`、`strength`、`period` も維持しています。

録画はこれまでどおりiframe内のcanvasをReactが取り出して行います。Saveボタンの6秒録画・送信・MP4変換・QRコード発行はGlowingDrawing側の機能で、PaintCoreは関与しません。

## 新しいアプリを作る場合

標準UIでよければ `example-basic/src/ofApp.h` のようにPaintEditorを持ち、setup、draw、マウス、キー入力を転送します。レイヤーIDや履歴の接続は不要です。

独自UIを作る場合はPaintSessionだけを持ちます。スライダー変更でBrushSettingsを作り、ボタンでundoやaddLayerを呼びます。Reactを使っても、ofxGuiを使っても、独自の3D操作UIと組み合わせても、同じ操作窓口が使えます。

雲用ブラシならBrushのスタンプを画像へ差し替えます。詳細コールバックの `StampInfo` から、同じ座標でも異なるスタンプを区別できます。ただしコールバックはプレビュー更新で同じIDに再度呼ばれるので、3D用データを蓄積する際は重複登録を避けます。

独自操作のUndoは `HistoryAction<PaintSession>` を継承して実装し、操作が成功した後で `recordAction()` に渡します。コアの内部フィールドを無理に外へ公開する必要はありません。

付属UIは基本のペイント操作を接続済みですが、GlowingDrawingのUIを置き換えてはいません。新しいアプリで任意に採用する部品です。市松模様、ブラシカーソル、GPUメモリ全体の自動上限管理は今回追加していません。

共通コアの変更を他のアプリへ届けるには、ofxPaintCoreのコミットを更新し、各アプリのサブモジュール参照を更新して再ビルドします。現在の変更はまだコミット・pushしていません。

## 検証と手動確認

ターミナルでの回帰テストは `tests/run-windows.ps1 -Run` から実行します。OpenGLコンテキストを作り、FBOを読み戻して通常描画、透明消しゴム、全消し、レイヤー追加／削除、Undo/Redo、履歴上限を確認します。テスト結果は `obj/paintcore-tests/paintcore-test-result.txt` です。

旧来のEngine呼び出しと新しいSessionの入口へ同じ入力を与え、全テクセルの値を比較するテストもあります。これは同じ入力に対する画像の互換性を調べるもので、液タブの実入力、操作感、ノートPCのフレームレートを測るものではありません。

手動確認では、Windows版とWeb版で短い線・長い線・透明消しゴムを使い、Normal／Glowの追加・削除・Undo/Redoを一往復させてください。次にレイヤー順変更、Glowスライダー、既存のSaveとQRコードを確認します。これらの確認結果を受けて、必要な修正を行います。
