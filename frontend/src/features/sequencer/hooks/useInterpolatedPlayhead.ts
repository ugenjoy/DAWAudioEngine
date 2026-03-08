import { useRef, useEffect } from 'react'

/**
 * Interpolates the playhead position between WebSocket updates (10Hz)
 * using requestAnimationFrame for smooth 60fps canvas rendering.
 * Accepts refs to avoid triggering React re-renders on position updates.
 */
export function useInterpolatedPlayhead(
  playheadPosRef: React.RefObject<number>,
  playheadUpdateRef: React.RefObject<number>,
  playing: boolean,
): React.RefObject<number> {
  const interpolatedPos = useRef(playheadPosRef.current)
  const lastCount = useRef(playheadUpdateRef.current)
  const lastServerTime = useRef(0)

  useEffect(() => {
    if (!playing) {
      interpolatedPos.current = playheadPosRef.current
      return
    }

    lastCount.current = playheadUpdateRef.current
    lastServerTime.current = performance.now()

    let rafId: number
    function tick() {
      if (playheadUpdateRef.current !== lastCount.current) {
        lastCount.current = playheadUpdateRef.current
        lastServerTime.current = performance.now()
      }
      const elapsedSec = (performance.now() - lastServerTime.current) / 1000
      interpolatedPos.current = playheadPosRef.current + elapsedSec
      rafId = requestAnimationFrame(tick)
    }
    rafId = requestAnimationFrame(tick)
    return () => cancelAnimationFrame(rafId)
  }, [playing, playheadPosRef, playheadUpdateRef])

  return interpolatedPos
}
