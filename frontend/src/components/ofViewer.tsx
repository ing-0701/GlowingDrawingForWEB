import { useEffect, useRef, useState } from 'react'
import ColorPicker from './ColorPicker'
import './ofViewer.css'
import { rangeStyle } from './rangeStyle'

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
    paintBackgroundBrightness?: number
    paintColor?: number
    paintPenSize?: number
    paintPenAlpha?: number
    paintEraserSize?: number
    paintEraserAlpha?: number
    paintPenMode?: boolean
    paintCommands?: PaintCommand[]
    paintLayerState?: PaintLayerState
}

export default function OfViewer() {
    const [brightness, setBrightness] = useState(255)
    const [color, setColor] = useState(0)
    const [penSize, setPenSize] = useState(0.1)
    const [penAlpha, setPenAlpha] = useState(1)
    const [eraserSize, setEraserSize] = useState(0.1)
    const [eraserAlpha, setEraserAlpha] = useState(1)
    const [isPen, setIsPen] = useState(true)
    const [showUi, setShowUi] = useState(true)
    const [layerState, setLayerState] = useState<PaintLayerState | null>(null)
    const [layerMenuOpen, setLayerMenuOpen] = useState(false)

    const iframeRef = useRef<HTMLIFrameElement>(null)
    const draggedLayerIdRef = useRef<number | null>(null)

    useEffect(() => {
        const timerId = window.setInterval(() => {
            const paintWindow = iframeRef.current?.contentWindow as PaintWindow | null
            
            // 命令の処理待ちなら、変更前の状態でUIを上書きしない
            if((paintWindow?.paintCommands?.length ?? 0) > 0){
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

    function sendGlowParameter(
        layerId: number,
        parameter: keyof GlowParameters,
        value: number,
    ){
        sendCommand({
            type: 'setGlowParameter',
            layerId,
            parameter,
            value,
        })

        // C++からの返答を待つ間も、つまみをすぐに動かす
        setLayerState((previous) => {
            if(!previous) return previous
            
            return {
                ...previous,
                layers: previous.layers.map((layer) =>{
                    if(layer.id !== layerId || !layer.glow){
                        return layer
                    }

                    return{
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
                        type='button'
                        aria-expanded={showUi}
                        aria-controls='drawing-sidebar drawing-layer-panel'
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
                    }}
                />

                <aside
                    id='drawing-sidebar'
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
                                min={0.05}
                                max={1}
                                step={0.01}
                                value={isPen ? penSize : eraserSize}
                                style={rangeStyle(isPen ? penSize : eraserSize, 0.05, 1)}
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
                <aside
                    id="drawing-layer-panel"
                    className="drawing-layer-panel"
                    aria-label="レイヤー"
                    hidden={!showUi}
                >
                    <div className='drawing-layer-heading'>
                        <strong>Layers</strong>

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
                                        if(draggedLayerIdRef.current === null){
                                            return
                                        }

                                        event.preventDefault()
                                        event.dataTransfer.dropEffect = 'move'
                                    }}
                                    onDrop={(event) => {
                                        const draggedId = draggedLayerIdRef.current

                                        if(draggedId === null){
                                            return
                                        }

                                        event.preventDefault()
                                        draggedLayerIdRef.current = null

                                        if(draggedId === layer.id){
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
                                    label: 'sigma',
                                    min: 1.0,
                                    max: 3.0,
                                },
                                {
                                    key: 'strength',
                                    label: 'strength',
                                    min: 1.0,
                                    max: 3.0,
                                },
                                {
                                    key: 'period',
                                    label: 'period(秒)',
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
                                        {selectedGlow[setting.key].toFixed(2)}
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
                    ) : (
                            <p>Glowレイヤーを選択すると発光を調整できます。</p>

                    )}
                </aside>
            </div>
        </section>
    )
}
