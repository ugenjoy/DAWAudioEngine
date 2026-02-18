import { useEffect, useRef } from 'react'

interface TimelineProps {
  draw: (value: CanvasRenderingContext2D, width: number, height: number) => void
  onWheel?: (e: WheelEvent) => void
}

function Timeline({ draw, onWheel }: Readonly<TimelineProps>) {
  const canvasRef = useRef<HTMLCanvasElement>(null)

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return

    const ctx = canvas.getContext('2d')
    if (!ctx) return

    requestAnimationFrame(() => {
      const rect = canvas.getBoundingClientRect()
      const dpr = window.devicePixelRatio || 1

      canvas.width = rect.width * dpr
      canvas.height = rect.height * dpr

      ctx.scale(dpr, dpr)

      draw(ctx, canvas.width, canvas.height)
    })
  }, [draw])

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas || !onWheel) return

    canvas.addEventListener('wheel', onWheel, { passive: false })
    return () => canvas.removeEventListener('wheel', onWheel)
  }, [onWheel])

  return (
    <canvas
      ref={canvasRef}
      className="w-full h-full"
      style={{ imageRendering: 'pixelated' }}
    />
  )
}

export default Timeline
