import { useRef, useState } from 'react'

type PaintWindow = Window & {
    paintBackgroundBrightness?: number
    paintHue?: number
    paintPenSize?: number
    paintPenAlpha?: number
    paintEraserSize?: number
    paintEraserAlpha?: number
    paintPenMode?: boolean
}

export default function OfViewer() {
    const [brightness, setBrightness] = useState(255)
    const [hue, setHue] = useState(0)
    const [penSize, setPenSize] = useState(0.1)
    const [penAlpha, setPenAlpha] = useState(1)
    const [eraserSize, setEraserSize] = useState(0.1)
    const [eraserAlpha, setEraserAlpha] = useState(1)
    const [isPen, setIsPen] = useState(true)

    const iframeRef = useRef<HTMLIFrameElement>(null)

    function sendBrightness(value: number) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintBackgroundBrightness = value
        }
    }

    function sendHue(value: number) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintHue = value
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
                色相:{hue}
                <input
                    type="range"
                    min={0}
                    max={1}
                    step={0.01}
                    value={hue}
                    onChange={(event) => {
                        const value = event.currentTarget.valueAsNumber
                        setHue(value)
                        sendHue(value)
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

            <iframe
                ref={iframeRef}
                title="openFrameworksのペイント画面"
                src="/engine/index.html"
                allowFullScreen
                onLoad={(event) => {
                    const iframe = event.currentTarget;

                    sendBrightness(brightness);
                    sendHue(hue);
                    sendPenSize(penSize);
                    sendPenAlpha(penAlpha);
                    sendEraserSize(eraserSize);
                    sendEraserAlpha(eraserAlpha);
                    sendPenMode(isPen);

                    const content =
                        iframe.contentDocument?.getElementById('engine-content');

                    if (!content) return;

                    const adjustHeight = () => {
                        const height = content.getBoundingClientRect().height;
                        iframe.style.height = `${Math.ceil(height)}px`;
                    };

                    adjustHeight();

                    const observer = new ResizeObserver(adjustHeight);
                    observer.observe(content);

                    iframe.contentWindow?.addEventListener(
                        'pagehide',
                        () => observer.disconnect(),
                        { once: true },
                    );
                }}
                style={{
                    display: 'block',
                    width: '1058px',
                    height: '600px',
                    border: 'none',
                    background: '#fff',
                }}
            />
        </section>
    )
}