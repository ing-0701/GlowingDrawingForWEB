import { useRef, useState } from 'react'
import ColorPicker from './ColorPicker'
import './ofViewer.css'
import { rangeStyle } from './rangeStyle'

type PaintCommand = 'undo' | 'redo' | 'deleteAll'

type PaintWindow = Window & {
    paintBackgroundBrightness?: number
    paintColor?: number
    paintPenSize?: number
    paintPenAlpha?: number
    paintEraserSize?: number
    paintEraserAlpha?: number
    paintPenMode?: boolean
    paintGlowMode?: boolean
    paintCommands?: PaintCommand[]
}

export default function OfViewer() {
    const [brightness, setBrightness] = useState(255)
    const [color, setColor] = useState(0)
    const [penSize, setPenSize] = useState(0.1)
    const [penAlpha, setPenAlpha] = useState(1)
    const [eraserSize, setEraserSize] = useState(0.1)
    const [eraserAlpha, setEraserAlpha] = useState(1)
    const [isPen, setIsPen] = useState(true)
    const [isGlow, setIsGlow] = useState(false)
    const [showUi, setShowUi] = useState(true)

    const iframeRef = useRef<HTMLIFrameElement>(null)

    function sendBrightness(value: number) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintBackgroundBrightness = value
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

    function sendGlowMode(value: boolean) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintGlowMode = value
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
                        type="button"
                        className='drawing-glow-button'
                        aria-pressed={isGlow}
                        title={
                            isGlow
                                ? '現在：発光キャンバス（通常に切り替える）'
                                : '現在：通常キャンバス（発光に切り替える）'
                        }
                        onClick={() => {
                            const nextIsGlow = !isGlow
                            setIsGlow(nextIsGlow)
                            sendGlowMode(nextIsGlow)
                        }}
                    >
                        {isGlow ? 'GlowMode' : 'NormalMode'}
                    </button>

                    <button
                        type='button'
                        aria-expanded={showUi}
                        aria-controls='drawing-sidebar'
                        title={showUi ? 'UIを非表示にする' : 'UIを表示する'}
                        onClick={() => {
                            setShowUi((current) => !current)
                        }}
                    >
                        UI
                    </button>
                </div>
            </header>

            <div className={showUi ? 'drawing-body' : 'drawing-body drawing-body--ui-hidden'}>
                <iframe
                    className='drawing-frame'
                    ref={iframeRef}
                    title="openFrameworksのペイント画面"
                    src="/engine/index.html"
                    width={800}
                    height={600}
                    allowFullScreen
                    onLoad={() => {
                        sendBrightness(brightness);
                        sendColor(color);
                        sendPenSize(penSize);
                        sendPenAlpha(penAlpha);
                        sendEraserSize(eraserSize);
                        sendEraserAlpha(eraserAlpha);
                        sendPenMode(isPen);
                        sendGlowMode(isGlow);
                    }}
                />

                <aside
                    id='drawing-slidebar'
                    className='drawing-sidebar'
                    aria-label='描画設定'
                    hidden={!showUi}
                >
                    <ColorPicker
                        onChange={(nextColor) => {
                            setColor(nextColor)
                            sendColor(nextColor)
                        }}
                    />

                    <div className='drawing-controls'>
                        <label className='drawing-field'>
                            <span>
                                Size
                                :{isPen ? penSize : eraserSize}
                            </span>

                            <input
                                type="range"
                                min={0}
                                max={1}
                                step={0.01}
                                value={isPen ? penSize : eraserSize}
                                style={rangeStyle(isPen ? penSize : eraserSize)}
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
                                Alpha
                                :{isPen ? penAlpha : eraserAlpha}
                            </span>
                            <input
                                type="range"
                                min={0}
                                max={1}
                                step={0.01}
                                value={isPen ? penAlpha : eraserAlpha}
                                style={rangeStyle(isPen ? penAlpha : eraserAlpha)}
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

                        <div className='drawing-history'>
                            <button type="button" onClick={() => sendCommand('undo')}>
                                UNDO
                            </button>

                            <button type="button" onClick={() => sendCommand('redo')}>
                                REDO
                            </button>
                        </div>

                        <button type="button" onClick={() => sendCommand('deleteAll')}>
                            DELETE ALL
                        </button>

                        <label className='drawing-field'>
                            <span>
                                Background:{brightness}
                            </span>

                            <input
                                type="range"
                                min={0}
                                max={255}
                                step={1}
                                value={brightness}
                                style={rangeStyle(brightness, 0, 255)}
                                onChange={(event) => {
                                    const value = event.currentTarget.valueAsNumber
                                    setBrightness(value)
                                    sendBrightness(value)
                                }}
                            />
                        </label>
                    </div>
                </aside>
            </div>
        </section>
    )
}