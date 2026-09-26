import { useRef, useState } from 'react'

type PaintWindow = Window & {
    paintBackgroundBrightness?: number
    paintPenMode?: boolean
}

export default function OfViewer() {
    const [brightness, setBrightness] = useState(255)
    const [isPen, setIsPen] = useState(true)

    const iframeRef = useRef<HTMLIFrameElement>(null)

    function sendBrightness(value: number) {
        const paintWindow = iframeRef.current?.contentWindow

        if (paintWindow) {
            (paintWindow as PaintWindow).paintBackgroundBrightness = value
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