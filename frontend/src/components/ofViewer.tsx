import { useEffect, useRef, useState } from 'react'
import ColorPicker from './ColorPicker'
import './ofViewer.css'
import { rangeStyle } from './rangeStyle'
import { QRCodeSVG } from 'qrcode.react'

type GlowParameters = {
    sigma: number
    strength: number
    period: number
}

type PaintCommand =
    | 'undo'
    | 'redo'
    | 'deleteAll'
    | 'addNormal'
    | 'addGlow'
    | { type: 'selectLayer'; layerId: number }
    | { type: 'deleteLayer'; layerId: number }
    | {
        type: 'moveLayer'
        layerId: number
        targetLayerId: number
    }
    | {
        type: 'setGlowParameter'
        layerId: number
        parameter: keyof GlowParameters
        value: number
    }

type PaintLayer = {
    id: number
    name: string
    type: 'Normal' | 'Glow'
    glow: GlowParameters | null
}

type PaintLayerState = {
    selectedLayerId: number
    layers: PaintLayer[]
}

type PaintWindow = Window & {
    paintBackgroundColor?: number
    paintColor?: number
    paintPenSize?: number
    paintPenAlpha?: number
    paintEraserSize?: number
    paintEraserAlpha?: number
    paintPenMode?: boolean
    paintCommands?: PaintCommand[]
    paintLayerState?: PaintLayerState
}

function BrushSizePreview({ size }: { size: number }) {
    const radius = size * 25
    const diameter = radius * 2

    return (
        <div
            className="drawing-size-preview"
            role="img"
            aria-label={`ブラシ半径 ${radius.toFixed(2)} ピクセル`}
        >
            <div
                className="drawing-size-preview-circle"
                style={{
                    width: diameter,
                    height: diameter,
                }}
            />
        </div>
    )
}

function toDisplayValue(value: number, min: number, max: number): number {
    const ratio = Math.max(0, Math.min(1, (value - min) / (max - min)))
    return Math.round(1 + ratio * 99)
}

export default function OfViewer() {
    const [backgroundColor, setBackgroundColor] = useState(0xffffff)
    const [backgroundPanelOpen, setBackgroundPanelOpen] = useState(false)
    const [color, setColor] = useState(0)
    const [penSize, setPenSize] = useState(0.1)
    const [penAlpha, setPenAlpha] = useState(1)
    const [eraserSize, setEraserSize] = useState(0.1)
    const [eraserAlpha, setEraserAlpha] = useState(1)
    const [isPen, setIsPen] = useState(true)
    const [brushPanelOpen, setBrushPanelOpen] = useState(false)
    const [layerPanelOpen, setLayerPanelOpen] = useState(false)
    const [layerState, setLayerState] = useState<PaintLayerState | null>(null)
    const [layerMenuOpen, setLayerMenuOpen] = useState(false)
    const [qrPanelOpen, setQrPanelOpen] = useState(false)

    const iframeRef = useRef<HTMLIFrameElement>(null)
    const draggedLayerIdRef = useRef<number | null>(null)

    const [isRecording, setIsRecording] = useState(false)
    const [recordingError, setRecordingError] = useState('')
    const [videoUrl, setVideoUrl] = useState<string | null>(null)
    const [isUploading, setIsUploading] = useState(false)
    const [downloadUrl, setDownloadUrl] = useState<string | null>(null)

    const [isSizeAdjusting, setIsSizeAdjusting] = useState(false)
    const [manualOpen, setManualOpen] = useState(false)

    useEffect(() => {
        const timerId = window.setInterval(() => {
            const paintWindow = iframeRef.current?.contentWindow as PaintWindow | null

            // 命令の処理待ちなら、変更前の状態でUIを上書きしない
            if ((paintWindow?.paintCommands?.length ?? 0) > 0) {
                return
            }

            const nextState = paintWindow?.paintLayerState

            if (!nextState) {
                setLayerState(null)
                return
            }

            setLayerState(nextState)
        }, 100)

        return () => {
            window.clearInterval(timerId)
        }
    }, [])

    useEffect(() => {
        if (!videoUrl) return

        return () => {
            URL.revokeObjectURL(videoUrl)
        }
    }, [videoUrl])

    function sendBackgroundColor(value: number) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintBackgroundColor = value
        }
    }

    function sendColor(value: number) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintColor = value
        }
    }

    function sendPenSize(value: number) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintPenSize = value
        }
    }

    function sendPenAlpha(value: number) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintPenAlpha = value
        }
    }

    function sendEraserSize(value: number) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintEraserSize = value
        }
    }

    function sendEraserAlpha(value: number) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintEraserAlpha = value
        }
    }

    function sendPenMode(value: boolean) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintPenMode = value
        }
    }

    function sendCommand(command: PaintCommand) {
        const paintWindow = iframeRef.current?.contentWindow as PaintWindow | null

        if (!paintWindow) return

        if (!paintWindow.paintCommands) {
            paintWindow.paintCommands = []
        }

        paintWindow.paintCommands.push(command)
    }

    function sendGlowParameter(
        layerId: number,
        parameter: keyof GlowParameters,
        value: number,
    ) {
        sendCommand({
            type: 'setGlowParameter',
            layerId,
            parameter,
            value,
        })

        // C++からの返答を待つ間も、つまみをすぐに動かす
        setLayerState((previous) => {
            if (!previous) return previous

            return {
                ...previous,
                layers: previous.layers.map((layer) => {
                    if (layer.id !== layerId || !layer.glow) {
                        return layer
                    }

                    return {
                        ...layer,
                        glow: {
                            ...layer.glow,
                            [parameter]: value,
                        }
                    }
                }),
            }
        })
    }

    const selectedLayer = layerState?.layers.find(
        (layer) => layer.id === layerState.selectedLayerId
    )

    const selectedGlow = selectedLayer?.glow

    async function saveVideo() {
        if (import.meta.env.VITE_OFFLINE_DEMO === 'true') {
            window.alert('この機能は現在利用できません')
            return
        }

        if (isRecording) return

        setIsRecording(true)
        setRecordingError('')
        setVideoUrl(null)
        setDownloadUrl(null)
        setIsUploading(false)

        let stream: MediaStream | null = null
        let stopTimer: number | undefined

        try {
            // 同じサイト内のiframeから、oFのcanvasを取得する
            const canvas =
                iframeRef.current?.contentDocument
                    ?.querySelector<HTMLCanvasElement>('#canvas')

            if (!canvas) {
                throw new Error('ペイント画面がまだ読み込まれていません。')
            }

            const mimeType = 'video/webm;codecs=vp8'

            if (
                typeof MediaRecorder === 'undefined' ||
                !MediaRecorder.isTypeSupported(mimeType)
            ) {
                throw new Error('このブラウザは今回の録画形式に対応していません。')
            }

            // canvasから、最大30fpsを指定して映像を取り出す
            stream = canvas.captureStream(30)

            const recorder = new MediaRecorder(stream, {
                mimeType,
                videoBitsPerSecond: 4_000_000,
            })

            const chunks: Blob[] = []

            const video = await new Promise<Blob>((resolve, reject) => {
                recorder.ondataavailable = (event) => {
                    if (event.data.size > 0) {
                        chunks.push(event.data)
                    }
                }

                recorder.onerror = () => {
                    reject(new Error('録画中にエラーが発生しました。'))
                }

                recorder.onstop = () => {
                    const blob = new Blob(chunks, {
                        type: recorder.mimeType,
                    })

                    if (blob.size === 0) {
                        reject(new Error('動画データを取得できませんでした。'))
                        return
                    }

                    resolve(blob)
                }

                recorder.start()

                stopTimer = window.setTimeout(() => {
                    if (recorder.state === 'recording') {
                        recorder.stop()
                    }
                }, 6000)
            })

            setVideoUrl(URL.createObjectURL(video))

            // 録画は終わったので、映像取得をここで止める
            stream?.getTracks().forEach((track) => track.stop())

            setIsUploading(true)

            const formData = new FormData()
            formData.append('file', video, 'painting.webm')

            const response = await fetch('/api/videos', {
                method: 'POST',
                body: formData,
            })

            if (!response.ok) {
                throw new Error(
                    `動画の送信に失敗しました（HTTP ${response.status}）。`
                )
            }

            const result = await response.json()

            if (typeof result.download_path !== 'string') {
                throw new Error('サーバーから動画の取得先が返されませんでした。')
            }

            const url = new URL(
                result.download_path,
                window.location.origin
            )

            setDownloadUrl(url.href)

        } catch (error) {
            setRecordingError(
                error instanceof Error
                    ? error.message
                    : '録画に失敗しました。'
            )
        } finally {
            if (stopTimer !== undefined) {
                window.clearTimeout(stopTimer)
            }

            // 映像の取り出しも終了する
            stream?.getTracks().forEach((track) => track.stop())

            setIsUploading(false)
            setIsRecording(false)
        }
    }

    return (
        <section className='drawing-app'>
            <header className='drawing-header'>
                <h1 className='drawing-title'>
                    GlowingDrawing
                </h1>

                <div className='drawing-header-actions'>
                    <button
                        type="button"
                        className="drawing-manual-button"
                        aria-haspopup="dialog"
                        onClick={() => setManualOpen(true)}
                    >
                        説明書
                    </button>

                    <button
                        type="button"
                        aria-expanded={brushPanelOpen}
                        aria-controls="drawing-brush-panel"
                        onClick={() => setBrushPanelOpen((open) => !open)}
                    >
                        Brush
                    </button>

                    <button
                        type="button"
                        aria-expanded={layerPanelOpen}
                        aria-controls="drawing-layer-panel"
                        onClick={() => setLayerPanelOpen((open) => !open)}
                    >
                        Layer
                    </button>

                    <button
                        type="button"
                        aria-expanded={backgroundPanelOpen}
                        aria-controls="drawing-background-panel"
                        onClick={() => setBackgroundPanelOpen((open) => !open)}
                    >
                        Background
                    </button>

                    <button
                        type='button'
                        aria-expanded={qrPanelOpen}
                        aria-controls='drawing-qr-panel'
                        onClick={() => setQrPanelOpen((open) => !open)}
                    >
                        QRcode
                    </button>
                </div>
            </header>

            <div className='drawing-body'>
                <iframe
                    className='drawing-frame'
                    ref={iframeRef}
                    title="openFrameworksのペイント画面"
                    src="/engine/index.html"
                    width={800}
                    height={600}
                    allowFullScreen
                    onLoad={() => {
                        sendBackgroundColor(backgroundColor);
                        sendColor(color);
                        sendPenSize(penSize);
                        sendPenAlpha(penAlpha);
                        sendEraserSize(eraserSize);
                        sendEraserAlpha(eraserAlpha);
                        sendPenMode(isPen);
                    }}
                />

                <aside
                    className='drawing-button-panel'
                    aria-label='描画操作'
                >
                    <button
                        type="button"
                        title={
                            isPen
                                ? '現在：ペン（消しゴムに切り替える）'
                                : '現在：消しゴム（ペンに切り替える）'
                        }
                        onClick={() => {
                            const nextIsPen = !isPen
                            setIsPen(nextIsPen)
                            sendPenMode(nextIsPen)
                        }}
                    >
                        {isPen ? 'Pen' : 'Eraser'}
                    </button>

                    <button
                        type='button'
                        onClick={() => sendCommand('undo')}
                    >
                        Undo
                    </button>

                    <button
                        type='button'
                        onClick={() => sendCommand('redo')}
                    >
                        Redo
                    </button>

                    <button
                        type='button'
                        onClick={() => sendCommand('deleteAll')}
                    >
                        Delete
                    </button>

                    <button
                        type='button'
                        onClick={() => void saveVideo()}
                        disabled={isRecording || !layerState}
                    >
                        {isUploading
                            ? '送信中…'
                            : isRecording
                                ? '録画中…'
                                : 'Save'}
                    </button>
                </aside>

                <aside
                    id="drawing-brush-panel"
                    className="drawing-sidebar"
                    aria-label="ブラシ設定"
                    hidden={!brushPanelOpen}
                >
                    <strong className="drawing-brush-heading">ブラシ設定</strong>
                    <div
                        className="drawing-brush-color"
                        inert={!isPen}
                    >
                        <ColorPicker
                            onChange={(nextColor) => {
                                setColor(nextColor)
                                sendColor(nextColor)
                            }}
                        />
                    </div>

                    <div className="drawing-controls">
                        <div className="drawing-line-panel">
                            <label className='drawing-field'>
                                <span>
                                    太さ
                                    :{toDisplayValue(isPen ? penSize : eraserSize, 0.05, 1)}
                                </span>

                                <input
                                    className="drawing-size-slider"
                                    type="range"
                                    min={0.05}
                                    max={1}
                                    step={0.01}
                                    value={isPen ? penSize : eraserSize}
                                    style={rangeStyle(isPen ? penSize : eraserSize, 0.05, 1)}
                                    onPointerDown={(event) => {
                                        if (!event.isPrimary || event.button !== 0) {
                                            return
                                        }

                                        event.currentTarget.setPointerCapture(event.pointerId)
                                        setIsSizeAdjusting(true)
                                    }}
                                    onPointerUp={() => {
                                        setIsSizeAdjusting(false)
                                    }}
                                    onPointerCancel={() => {
                                        setIsSizeAdjusting(false)
                                    }}
                                    onLostPointerCapture={() => {
                                        setIsSizeAdjusting(false)
                                    }}
                                    onBlur={() => {
                                        setIsSizeAdjusting(false)
                                    }}
                                    onChange={(event) => {
                                        const value = event.currentTarget.valueAsNumber
                                        if (isPen) {
                                            setPenSize(value)
                                            sendPenSize(value)
                                        } else {
                                            setEraserSize(value)
                                            sendEraserSize(value)
                                        }
                                    }}
                                />
                            </label>

                            <label className='drawing-field'>
                                <span>
                                    不透明度
                                    :{toDisplayValue(isPen ? penAlpha : eraserAlpha, 0.01, 1)}
                                </span>
                                <input
                                    type="range"
                                    min={0.01}
                                    max={1}
                                    step={0.01}
                                    value={isPen ? penAlpha : eraserAlpha}
                                    style={rangeStyle(isPen ? penAlpha : eraserAlpha, 0.01, 1)}
                                    onChange={(event) => {
                                        const value = event.currentTarget.valueAsNumber

                                        if (isPen) {
                                            setPenAlpha(value)
                                            sendPenAlpha(value)
                                        } else {
                                            setEraserAlpha(value)
                                            sendEraserAlpha(value)
                                        }
                                    }}
                                />
                            </label>
                            {isSizeAdjusting && (
                                <BrushSizePreview
                                    size={isPen ? penSize : eraserSize}
                                />
                            )}
                        </div>

                    </div>
                </aside>
                <aside
                    id="drawing-layer-panel"
                    className="drawing-layer-panel"
                    aria-label="レイヤー"
                    hidden={!layerPanelOpen}
                >
                    <div className='drawing-layer-heading'>
                        <strong>レイヤー</strong>

                        <button
                            type='button'
                            aria-label='レイヤー追加メニュー'
                            aria-expanded={layerMenuOpen}
                            aria-controls='drawing-layer-menu'
                            disabled={layerState === null}
                            onClick={() => setLayerMenuOpen((open) => !open)}
                        >
                            {layerMenuOpen ? '閉じる' : '＋'}
                        </button>
                    </div>

                    {layerMenuOpen && (
                        <div id="drawing-layer-menu" className="drawing-layer-menu">
                            <button
                                type="button"
                                onClick={() => {
                                    sendCommand('addNormal')
                                    setLayerMenuOpen(false)
                                }}
                            >
                                Normal
                            </button>
                            <button
                                type="button"
                                onClick={() => {
                                    sendCommand('addGlow')
                                    setLayerMenuOpen(false)
                                }}
                            >
                                Glow
                            </button>
                        </div>
                    )}

                    {layerState === null ? (
                        <p>レイヤー情報を待っています…</p>
                    ) : (
                        <ul className="drawing-layer-list">
                            {layerState.layers.map((layer) => (
                                <li
                                    key={layer.id}
                                    className={
                                        layer.id === layerState.selectedLayerId
                                            ? 'drawing-layer-row is-selected'
                                            : 'drawing-layer-row'
                                    }
                                    onDragOver={(event) => {
                                        if (draggedLayerIdRef.current === null) {
                                            return
                                        }

                                        event.preventDefault()
                                        event.dataTransfer.dropEffect = 'move'
                                    }}
                                    onDrop={(event) => {
                                        const draggedId = draggedLayerIdRef.current

                                        if (draggedId === null) {
                                            return
                                        }

                                        event.preventDefault()
                                        draggedLayerIdRef.current = null

                                        if (draggedId === layer.id) {
                                            return
                                        }

                                        sendCommand({
                                            type: 'moveLayer',
                                            layerId: draggedId,
                                            targetLayerId: layer.id,
                                        })
                                    }}
                                >
                                    <button
                                        type="button"
                                        className="drawing-layer-select"
                                        draggable
                                        aria-pressed={
                                            layer.id === layerState.selectedLayerId
                                        }
                                        onClick={() => sendCommand({
                                            type: 'selectLayer',
                                            layerId: layer.id,
                                        })}
                                        onDragStart={(event) => {
                                            draggedLayerIdRef.current = layer.id

                                            event.dataTransfer.effectAllowed = 'move'
                                            event.dataTransfer.setData(
                                                'text/plain',
                                                String(layer.id)
                                            )

                                            sendCommand({
                                                type: 'selectLayer',
                                                layerId: layer.id,
                                            })
                                        }}

                                        onDragEnd={() => {
                                            draggedLayerIdRef.current = null
                                        }}
                                    >
                                        <span>{layer.name}</span>
                                        <small>{layer.type}Layer</small>
                                    </button>

                                    <button
                                        type="button"
                                        className="drawing-layer-delete"
                                        aria-label={`${layer.name}を削除`}
                                        disabled={layerState.layers.length <= 1}
                                        onClick={() => sendCommand({
                                            type: 'deleteLayer',
                                            layerId: layer.id,
                                        })}
                                    >
                                        ×
                                    </button>
                                </li>
                            ))}
                        </ul>
                    )}

                    {selectedLayer && selectedGlow ? (
                        <div className='drawing-glow-controls'>
                            <strong>{selectedLayer.name}の発光設定</strong>

                            {([
                                {
                                    key: 'sigma',
                                    label: '光の広がり',
                                    min: 1.0,
                                    max: 4.0,
                                },
                                {
                                    key: 'strength',
                                    label: '光の強さ',
                                    min: 1.5,
                                    max: 7.5,
                                },
                                {
                                    key: 'period',
                                    label: '点滅周期(秒)',
                                    min: 0.3,
                                    max: 3.0,
                                },
                            ] as const).map((setting) => (
                                <label
                                    key={setting.key}
                                    className='drawing-field'
                                >
                                    <span>
                                        {setting.label}:
                                        {setting.key === 'period'
                                        ? selectedGlow[setting.key].toFixed(2)
                                        : toDisplayValue(
                                            selectedGlow[setting.key],
                                            setting.min,
                                            setting.max,
                                        )}
                                    </span>

                                    <input
                                        type='range'
                                        min={setting.min}
                                        max={setting.max}
                                        step={0.01}
                                        value={selectedGlow[setting.key]}
                                        style={rangeStyle(
                                            selectedGlow[setting.key],
                                            setting.min,
                                            setting.max
                                        )}
                                        onChange={(event) => {
                                            sendGlowParameter(
                                                selectedLayer.id,
                                                setting.key,
                                                event.currentTarget.valueAsNumber,
                                            )
                                        }}
                                    />
                                </label>
                            ))}
                        </div>
                    ) : null}
                </aside>

                <aside
                    id="drawing-background-panel"
                    className="drawing-background-panel"
                    aria-label="背景色"
                    hidden={!backgroundPanelOpen}
                >
                    <strong>背景色</strong>

                    <ColorPicker
                        initialHsv={{
                            hue: 0,
                            saturation: 0,
                            value: 1,
                        }}
                        onChange={(nextColor) => {
                            setBackgroundColor(nextColor)
                            sendBackgroundColor(nextColor)
                        }}
                    />
                </aside>

                <aside
                    id='drawing-qr-panel'
                    className='drawing-qr-panel'
                    aria-label='保存結果'
                    hidden={!qrPanelOpen}
                >
                    <strong>QRコード</strong>

                    {isRecording && (
                        <p>
                            {isUploading
                                ? 'QRコードを用意しています…'
                                : '録画しています…'}
                        </p>
                    )}

                    {!isRecording &&
                        !videoUrl &&
                        !downloadUrl &&
                        !recordingError && (
                            <p>まだ保存されていません</p>
                        )}

                    {recordingError && (
                        <p role="alert">{recordingError}</p>
                    )}

                    {downloadUrl && (
                        <>
                            <p>
                                保存が完了しました。
                                スマホでQRコードを読み取ってください。
                            </p>

                            <QRCodeSVG
                                value={downloadUrl}
                                size={200}
                                marginSize={4}
                                bgColor="#ffffff"
                                fgColor="#000000"
                                title="作品動画のダウンロード"
                            />
                        </>
                    )}

                </aside>
            </div>

            {/* アプリ公開・アンケート・SNSの案内 */}
            <div className="drawing-info-panels">
                <div className="drawing-info-panel">
                    <p>
                        改善＆機能追加をして<br />
                        WEBアプリとしての公開を<br />
                        計画しています！
                    </p>
                </div>

                <div className="drawing-info-panel">
                    <img
                        src="/images/qrcode_forms.png"
                        alt="ご意見・ご感想フォームのQRコード"
                    />
                    <p>
                        ◀ご意見＆ご感想<br />
                        ㅤ是非お気軽に！
                    </p>
                </div>

                <div className="drawing-info-panel">
                    <img
                        src="/images/qrcode_x.png"
                        alt="XアカウントのQRコード"
                    />
                    <p>
                        ◀リリースは<br />
                        ㅤこちらのアカウントから<br />
                        ㅤご報告します！
                    </p>
                </div>
            </div>

            {manualOpen && (
                <div
                    className="drawing-manual-overlay"
                    onClick={() => setManualOpen(false)}
                >
                    <div
                        className="drawing-manual-dialog"
                        role="dialog"
                        aria-modal="true"
                        aria-label="説明書"
                        onClick={(event) => event.stopPropagation()}
                    >
                        <img
                            src="/images/manual.png"
                            alt="GlowingDrawingの説明書"
                        />
                    </div>
                </div>
            )}
        </section>
    )
}

