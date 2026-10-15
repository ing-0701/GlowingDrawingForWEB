import type { CSSProperties } from 'react'

type RangeStyle = CSSProperties & {
    '--range-background': string
}

export function rangeStyle(
    value: number,
    min = 0,
    max = 1,
): RangeStyle {
    const ratio = Math.max(
        0,
        Math.min(1, (value - min) / (max - min)),
    )

    // CSS側のつまみ幅16pxに合わせる
    const thumbWidth = 16

    // つまみの中心位置。
    // つまみの幅があるため、単純な百分率から少し補正する。
    const position =
        `calc(${ratio * 100}% + ${thumbWidth / 2 - ratio * thumbWidth}px)`

    return {
        '--range-background':
            ratio === 0
                ? 'none'
                : `
                    linear-gradient(
                        to right,
                        transparent 0%,
                        transparent ${position},
                        #222 ${position},
                        #222 100%
                    ),
                    linear-gradient(
                        to right,
                        #383838 0%,
                        #ffffffee ${position},
                        #fff 100%
                    )
                `,
    }
}
