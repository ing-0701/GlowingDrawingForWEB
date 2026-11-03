import { useState } from 'react'
import type { PointerEvent } from 'react'

//propsとは、親コンポーネントから渡される値
//今回はonChangeという関数を受け取っている
//親側にある色設定の関数をこちらからコールバック関数として使うことができる
type Props = {
    onChange: (color: number) => void
    initialHsv?: Hsv
}

type Hsv = {
    hue: number
    saturation: number
    value: number
}

//値を0~1に収める関数
function clamp(value: number) {
    return Math.max(0, Math.min(1, value))
}

// HSV（各0〜1）を、0xRRGGBB形式の整数に変換する
function toRgbNumber({ hue, saturation, value }: Hsv) {
    const h = (hue % 1) * 6
    const chroma = value * saturation //クロマとはRGBの3つの値のうち、最大値と最小値の差
    const x = chroma * (1 - Math.abs((h % 2) - 1))
    const m = value - chroma
    // RGBのうち、最大の要素の値がchroma、中間の要素の値がx、最小の要素の値が0
    // これがRGBの理想的な「差」の関係（比率ではない）
    // 輝度とは、RGBのうち最大の要素の値である
    // よってRGBの全てにmを足せば正しく変換できる

    let r = 0
    let g = 0
    let b = 0

    if (h < 1) {
        r = chroma
        g = x
    } else if (h < 2) {
        r = x
        g = chroma
    } else if (h < 3) {
        g = chroma
        b = x
    } else if (h < 4) {
        g = x
        b = chroma
    } else if (h < 5) {
        r = x
        b = chroma
    } else {
        r = chroma
        b = x
    }

    const red = Math.round((r + m) * 255)
    const green = Math.round((g + m) * 255)
    const blue = Math.round((b + m) * 255)

    return (red << 16) | (green << 8) | blue
}

export default function ColorPicker({
        onChange,
    initialHsv = {
        hue: 0,
        saturation: 0,
        value: 0,
    },
}: Props) {
    const [hsv, setHsv] = useState<Hsv>(initialHsv)

    function updateColor(next: Hsv) {
        setHsv(next)
        onChange(toRgbNumber(next))
    }

    //マウスの位置とカラーピッカーの位置を計算して彩度と明度を取得
    //色相はすでに決まっているため、この関数が呼ばれたときペンの色が決まる
    function pick(event: PointerEvent<HTMLDivElement>) {
        const rect = event.currentTarget.getBoundingClientRect()

        updateColor({
            ...hsv,
            saturation: clamp((event.clientX - rect.left) / rect.width),
            value: 1 - clamp((event.clientY - rect.top) / rect.height),
        })
    }

    //カラーコードの文字列を生成
    // const colorHex =
    //     `#${toRgbNumber(hsv).toString(16).padStart(6, '0')}`

    return (
        <div style={{ width: 256, flexShrink: 0, textAlign: 'left' }}>
            <div
                role="group"
                aria-label="彩度と明度。左右キーで彩度、上下キーで明度を変更"
                tabIndex={0}
                onPointerDown={(event) => {
                    if (event.button !== 0) return

                    event.currentTarget.focus()
                    event.currentTarget.setPointerCapture(event.pointerId)
                    pick(event)
                }}
                onPointerMove={(event) => {
                    if (
                        event.currentTarget.hasPointerCapture(event.pointerId)
                    ) {
                        pick(event)
                    }
                }}
                onPointerUp={(event) => {
                    if (
                        event.currentTarget.hasPointerCapture(event.pointerId)
                    ) {
                        pick(event)
                        event.currentTarget.releasePointerCapture(
                            event.pointerId,
                        )
                    }
                }}
                onKeyDown={(event) => {
                    const step = event.shiftKey ? 0.1 : 0.01
                    let { saturation, value } = hsv

                    switch (event.key) {
                        case 'ArrowLeft':
                            saturation -= step
                            break
                        case 'ArrowRight':
                            saturation += step
                            break
                        case 'ArrowUp':
                            value += step
                            break
                        case 'ArrowDown':
                            value -= step
                            break
                        default:
                            return
                    }

                    event.preventDefault()
                    updateColor({
                        ...hsv,
                        saturation: clamp(saturation),
                        value: clamp(value),
                    })
                }}
                style={{
                    position: 'relative',
                    width: 256,
                    height: 256,
                    overflow: 'hidden',
                    touchAction: 'none',
                    userSelect: 'none',
                    cursor: 'crosshair',
                    background: [
                        'linear-gradient(to top, #000, transparent)',
                        'linear-gradient(to right, #fff, transparent)',
                        `hsl(${hsv.hue * 360}, 100%, 50%)`,
                    ].join(', '),
                }}
            >
                <span
                    aria-hidden="true"
                    style={{
                        position: 'absolute',
                        left: `${hsv.saturation * 100}%`,
                        top: `${(1 - hsv.value) * 100}%`,
                        transform: 'translate(-50%, -50%)',
                        width: 12,
                        height: 12,
                        borderRadius: '50%',
                        border: '2px solid white',
                        boxShadow: '0 0 0 1px black',
                        pointerEvents: 'none',
                    }}
                />
            </div>

            {/* カラーコードのテキスト表示 */}
            {/* 当面使わなさそうなのでいったんコメントアウト */}
            {/* <div
                style={{
                    display: 'flex',
                    alignItems: 'center',
                    gap: 8,
                    marginTop: 8,
                    padding: '0 12px',
                }}
            >
                <span
                    aria-hidden="true"
                    style={{
                        display: 'inline-block',
                        width: 24,
                        height: 24,
                        background: colorHex,
                        border: '1px solid #fff',
                    }}
                />
                <output aria-label="選択色">{colorHex}</output>
            </div> */}

            <label style={{ display: 'block', marginTop: 8, padding: '0 12px' }}>
                <input
                    aria-label="色相"
                    className="hue-range"
                    type="range"
                    min={0}
                    max={1}
                    step={0.001}
                    value={hsv.hue}
                    onChange={(event) => {
                        updateColor({
                            ...hsv,
                            hue: event.currentTarget.valueAsNumber,
                        })
                    }}
                />
            </label>
        </div>
    )
}
