import { useEffect, useRef } from 'react'

interface TimelineProps {
  draw: (value: CanvasRenderingContext2D, width: number, height: number) => void
  playing?: boolean
  continuousRender?: boolean
  onWheel?: (e: WheelEvent) => void
  onClick?: (e: MouseEvent) => void
  onMouseMove?: (e: MouseEvent) => void
  onMouseUp?: (e: MouseEvent) => void
  onContextMenu?: (e: MouseEvent) => void
  onKeyDown?: (e: KeyboardEvent) => void
}

function Timeline({
  draw,
  playing,
  continuousRender,
  onWheel,
  onClick,
  onMouseMove,
  onMouseUp,
  onContextMenu,
  onKeyDown,
}: Readonly<TimelineProps>) {
  const canvasRef = useRef<HTMLCanvasElement>(null)
  const drawRef = useRef(draw)
  drawRef.current = draw

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return

    const ctx = canvas.getContext('2d')
    if (!ctx) return

    let rafId: number

    function render() {
      const rect = canvas!.getBoundingClientRect()
      const dpr = window.devicePixelRatio || 1

      canvas!.width = rect.width * dpr
      canvas!.height = rect.height * dpr

      ctx!.scale(dpr, dpr)

      drawRef.current(ctx!, canvas!.width, canvas!.height)

      if (playing || continuousRender) {
        rafId = requestAnimationFrame(render)
      }
    }

    rafId = requestAnimationFrame(render)
    return () => cancelAnimationFrame(rafId)
  }, [draw, playing, continuousRender])

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas || !onWheel) return
    canvas.addEventListener('wheel', onWheel, { passive: false })
    return () => canvas.removeEventListener('wheel', onWheel)
  }, [onWheel])

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas || !onClick) return
    canvas.addEventListener('mousedown', onClick)
    return () => canvas.removeEventListener('mousedown', onClick)
  }, [onClick])

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas || !onMouseMove) return
    canvas.addEventListener('mousemove', onMouseMove)
    return () => canvas.removeEventListener('mousemove', onMouseMove)
  }, [onMouseMove])

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas || !onMouseUp) return
    canvas.addEventListener('mouseup', onMouseUp)
    return () => canvas.removeEventListener('mouseup', onMouseUp)
  }, [onMouseUp])

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas || !onContextMenu) return
    canvas.addEventListener('contextmenu', onContextMenu)
    return () => canvas.removeEventListener('contextmenu', onContextMenu)
  }, [onContextMenu])

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas || !onKeyDown) return
    globalThis.addEventListener('keydown', onKeyDown)
    return () => globalThis.removeEventListener('keydown', onKeyDown)
  }, [onKeyDown])

  return (
    <canvas
      ref={canvasRef}
      className="w-full h-full"
      style={{ imageRendering: 'pixelated' }}
    />
  )
}

export default Timeline
