import { useRef, useEffect } from 'react'

/**
 * Interpolates the playhead position between WebSocket updates (10Hz)
 * using requestAnimationFrame for smooth 60fps canvas rendering.
 * Returns a ref (not state) to avoid triggering React re-renders.
 */
export function useInterpolatedPlayhead(
  serverPos: number,
  playing: boolean,
): React.RefObject<number> {
  const interpolatedPos = useRef(serverPos)
  const lastServerPos = useRef(serverPos)
  const lastServerTime = useRef(0)

  useEffect(() => {
    lastServerPos.current = serverPos
    lastServerTime.current = performance.now()
    if (!playing) {
      interpolatedPos.current = serverPos
    }
  }, [serverPos, playing])

  useEffect(() => {
    if (!playing) {
      interpolatedPos.current = lastServerPos.current
      return
    }

    let rafId: number
    function tick() {
      const elapsedSec = (performance.now() - lastServerTime.current) / 1000
      interpolatedPos.current = lastServerPos.current + elapsedSec
      rafId = requestAnimationFrame(tick)
    }
    rafId = requestAnimationFrame(tick)
    return () => cancelAnimationFrame(rafId)
  }, [playing])

  return interpolatedPos
}
