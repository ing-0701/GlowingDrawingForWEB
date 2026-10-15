import { useRef, useState } from 'react'
import ColorPicker from './ColorPicker'

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
        <section style={{ width: '100%', padding: '0 0' }}>
            <label style={{ display: 'block' }}>
                背景の明るさ:{brightness}
                <input
                    type="range"
                    min={0}
                    max={255}
                    step={1}
                    value={brightness}
                    onChange={(event) => {
                        const value = event.currentTarget.valueAsNumber
                        setBrightness(value)
                        sendBrightness(value)
                    }}
                />
            </label>

            <label style={{ display: 'block' }}>
                ペンの太さ:{penSize}
                <input
                    type="range"
                    min={0}
                    max={1}
                    step={0.01}
                    value={penSize}
                    onChange={(event) => {
                        const value = event.currentTarget.valueAsNumber
                        setPenSize(value)
                        sendPenSize(value)
                    }}
                />
            </label>

            <label style={{ display: 'block' }}>
                ペンの透明度:{penAlpha}
                <input
                    type="range"
                    min={0}
                    max={1}
                    step={0.01}
                    value={penAlpha}
                    onChange={(event) => {
                        const value = event.currentTarget.valueAsNumber
                        setPenAlpha(value)
                        sendPenAlpha(value)
                    }}
                />
            </label>

            <label style={{ display: 'block' }}>
                消しゴムの太さ:{eraserSize}
                <input
                    type="range"
                    min={0}
                    max={1}
                    step={0.01}
                    value={eraserSize}
                    onChange={(event) => {
                        const value = event.currentTarget.valueAsNumber
                        setEraserSize(value)
                        sendEraserSize(value)
                    }}
                />
            </label>

            <label style={{ display: 'block' }}>
                消しゴムの透明度:{eraserAlpha}
                <input
                    type="range"
                    min={0}
                    max={1}
                    step={0.01}
                    value={eraserAlpha}
                    onChange={(event) => {
                        const value = event.currentTarget.valueAsNumber
                        setEraserAlpha(value)
                        sendEraserAlpha(value)
                    }}
                />
            </label>

            <button
                type="button"
                onClick={() => {
                    const nextIsPen = !isPen
                    setIsPen(nextIsPen)
                    sendPenMode(nextIsPen)
                }}
            >
                {isPen
                    ? '現在：ペン（消しゴムに切り替える）'
                    : '現在：消しゴム（ペンに切り替える）'
                }
            </button>

            <button
                type="button"
                onClick={() => {
                    const nextIsGlow = !isGlow
                    setIsGlow(nextIsGlow)
                    sendGlowMode(nextIsGlow)
                }}
            >
                {isGlow
                    ? '現在：発光キャンバス（通常に切り替える）'
                    : '現在：通常キャンバス（発光に切り替える）'
                }
            </button>

            <div>
                <button type="button" onClick={() => sendCommand('undo')}>
                    UNDO
                </button>

                <button type="button" onClick={() => sendCommand('redo')}>
                    REDO
                </button>

                <button type="button" onClick={() => sendCommand('deleteAll')}>
                    全消し
                </button>
            </div>

            <div
                style={{
                    display: 'flex',
                    alignItems: 'flex-start',
                    gap: 0,
                    width: 'max-content',
                }}
            >
                <iframe
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
                    style={{
                        display: 'block',
                        width: '800px',
                        height: '600px',
                        flexShrink: 0,
                        border: 'none',
                        background: '#fff',
                    }}
                />

                <ColorPicker
                    onChange={(nextColor) => {
                        setColor(nextColor)
                        sendColor(nextColor)
                    }}
                />
            </div>
        </section>
    )
}