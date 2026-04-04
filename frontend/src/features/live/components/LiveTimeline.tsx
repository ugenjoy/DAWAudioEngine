import { useEffect, useRef } from 'react'
import { Song } from '@/shared/models/song'
import { useInterpolatedPlayhead } from '@/features/sequencer/hooks/useInterpolatedPlayhead'

const TRACK_COLORS = [
  '#3b82f6',
  '#ef4444',
  '#22c55e',
  '#f59e0b',
  '#a855f7',
  '#ec4899',
  '#14b8a6',
  '#f97316',
]

interface Props {
  song: Song
  positionRef: React.RefObject<number>
  playheadUpdateRef: React.RefObject<number>
  playing: boolean
  pixelsPerSecond?: number
}

export function LiveTimeline({
  song,
  positionRef,
  playheadUpdateRef,
  playing,
  pixelsPerSecond = 50,
}: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null)
  const interpolatedPos = useInterpolatedPlayhead(positionRef, playheadUpdateRef, playing)

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return
    let rafId: number

    function resize() {
      if (!canvas) return
      const dpr = window.devicePixelRatio ?? 1
      const rect = canvas.getBoundingClientRect()
      canvas.width = rect.width * dpr
      canvas.height = rect.height * dpr
    }

    resize()
    const resizeObserver = new ResizeObserver(resize)
    resizeObserver.observe(canvas)

    function draw() {
      const ctx = canvas!.getContext('2d')
      if (!ctx) return

      const dpr = window.devicePixelRatio ?? 1
      const { width, height } = canvas!

      const position = interpolatedPos.current ?? 0
      const center = width / 2
      const trackHeight = 20 * dpr

      ctx.clearRect(0, 0, width, height)
      ctx.fillStyle = '#0f0f0f'
      ctx.fillRect(0, 0, width, height)

      song.tracks.forEach((track, trackIndex) => {
        const y = 20 * dpr + trackIndex * trackHeight
        const color = TRACK_COLORS[trackIndex % TRACK_COLORS.length]

        track.clips.forEach((clip) => {
          const clipX =
            center + (clip.position - position) * pixelsPerSecond * dpr
          const clipW = clip.duration * pixelsPerSecond * dpr
          if (clipX + clipW < 0 || clipX > width) return

          ctx.fillStyle = color
          ctx.globalAlpha = 0.85
          ctx.fillRect(clipX, y, clipW, trackHeight - 2 * dpr)
          ctx.globalAlpha = 1
          ctx.fillStyle = '#fff'
          ctx.font = '11px sans-serif'
          ctx.fillText(clip.name, clipX + 4, y + trackHeight / 2 + 4)
        })
      })

      // End position marker
      if (song.endPosition !== undefined) {
        const endX =
          center + (song.endPosition - position) * pixelsPerSecond * dpr
        if (endX >= 0 && endX <= width) {
          ctx.strokeStyle = '#f97316'
          ctx.lineWidth = 1.5 * dpr
          ctx.setLineDash([5 * dpr, 3 * dpr])
          ctx.beginPath()
          ctx.moveTo(endX, 0)
          ctx.lineTo(endX, height)
          ctx.stroke()
          ctx.setLineDash([])

          // Small triangle flag at top
          ctx.fillStyle = '#f97316'
          ctx.beginPath()
          ctx.moveTo(endX - 6 * dpr, 0)
          ctx.lineTo(endX + 6 * dpr, 0)
          ctx.lineTo(endX, 10 * dpr)
          ctx.closePath()
          ctx.fill()
        }
      }

      ctx.globalAlpha = 1
      ctx.strokeStyle = '#ffffff'
      ctx.lineWidth = 2 * dpr
      ctx.beginPath()
      ctx.moveTo(center, 0)
      ctx.lineTo(center, height)
      ctx.stroke()

      rafId = requestAnimationFrame(draw)
    }

    rafId = requestAnimationFrame(draw)
    return () => {
      cancelAnimationFrame(rafId)
      resizeObserver.disconnect()
    }
  }, [song, pixelsPerSecond])

  return <canvas ref={canvasRef} className="w-full h-full" />
}
