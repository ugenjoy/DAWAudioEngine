import { memo, useEffect, useRef } from 'react'

interface TimelineProps {
  draw: (value: CanvasRenderingContext2D, width: number, height: number) => void
  onWheel?: (e: WheelEvent) => void
  onClick?: (e: MouseEvent) => void
  onMouseMove?: (e: MouseEvent) => void
  onMouseUp?: (e: MouseEvent) => void
  onContextMenu?: (e: MouseEvent) => void
  onKeyDown?: (e: KeyboardEvent) => void
}

const Timeline = memo(function Timeline({
  draw,
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

  const onWheelRef = useRef(onWheel)
  const onClickRef = useRef(onClick)
  const onMouseMoveRef = useRef(onMouseMove)
  const onMouseUpRef = useRef(onMouseUp)
  const onContextMenuRef = useRef(onContextMenu)
  const onKeyDownRef = useRef(onKeyDown)
  onWheelRef.current = onWheel
  onClickRef.current = onClick
  onMouseMoveRef.current = onMouseMove
  onMouseUpRef.current = onMouseUp
  onContextMenuRef.current = onContextMenu
  onKeyDownRef.current = onKeyDown

  // Always-running render loop — reads draw from ref, no restart needed
  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return

    const ctx = canvas.getContext('2d')
    if (!ctx) return

    let rafId: number
    let lastWidth = 0
    let lastHeight = 0

    function render() {
      const rect = canvas!.getBoundingClientRect()
      const dpr = window.devicePixelRatio || 1
      const w = Math.round(rect.width * dpr)
      const h = Math.round(rect.height * dpr)

      if (lastWidth !== w || lastHeight !== h) {
        canvas!.width = w
        canvas!.height = h
        lastWidth = w
        lastHeight = h
      }

      ctx!.setTransform(dpr, 0, 0, dpr, 0, 0)
      drawRef.current(ctx!, rect.width, rect.height)

      rafId = requestAnimationFrame(render)
    }

    rafId = requestAnimationFrame(render)
    return () => cancelAnimationFrame(rafId)
  }, [])

  // Attach all event listeners once
  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return

    const handleWheel = (e: WheelEvent) => onWheelRef.current?.(e)
    const handleClick = (e: MouseEvent) => onClickRef.current?.(e)
    const handleMouseMove = (e: MouseEvent) => onMouseMoveRef.current?.(e)
    const handleMouseUp = (e: MouseEvent) => onMouseUpRef.current?.(e)
    const handleContextMenu = (e: MouseEvent) => onContextMenuRef.current?.(e)
    const handleKeyDown = (e: KeyboardEvent) => onKeyDownRef.current?.(e)

    canvas.addEventListener('wheel', handleWheel, { passive: false })
    canvas.addEventListener('mousedown', handleClick)
    canvas.addEventListener('mousemove', handleMouseMove)
    canvas.addEventListener('mouseup', handleMouseUp)
    canvas.addEventListener('contextmenu', handleContextMenu)
    globalThis.addEventListener('keydown', handleKeyDown)

    return () => {
      canvas.removeEventListener('wheel', handleWheel)
      canvas.removeEventListener('mousedown', handleClick)
      canvas.removeEventListener('mousemove', handleMouseMove)
      canvas.removeEventListener('mouseup', handleMouseUp)
      canvas.removeEventListener('contextmenu', handleContextMenu)
      globalThis.removeEventListener('keydown', handleKeyDown)
    }
  }, [])

  return (
    <canvas
      ref={canvasRef}
      className="w-full h-full"
      style={{ imageRendering: 'pixelated' }}
    />
  )
})

export default Timeline
