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
  const playingRef = useRef(playing)
  playingRef.current = playing

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

      const position = playingRef.current
        ? (interpolatedPos.current ?? 0)
        : (positionRef.current ?? 0)
      const center = width / 2
      const trackHeight = 20 * dpr
      const pps = pixelsPerSecond * dpr

      ctx.clearRect(0, 0, width, height)
      ctx.fillStyle = '#000000'
      ctx.fillRect(0, 0, width, height)

      // Active zone background and grid, bounded to [0, endPosition]
      {
        const beatDuration = 60 / song.tempo
        const visibleStart = position - center / pps
        const visibleEnd = position + (width - center) / pps
        const gridEnd = song.endPosition ?? visibleEnd

        const xStart = Math.max(0, center + (0 - position) * pps)
        const xEnd = Math.min(width, center + (gridEnd - position) * pps)

        if (xEnd > xStart) {
          ctx.fillStyle = '#0f0f0f'
          ctx.fillRect(xStart, 0, xEnd - xStart, height)
        }

        const firstBeat = Math.max(0, Math.floor(visibleStart / beatDuration))
        const lastBeat = Math.floor(Math.min(gridEnd, visibleEnd) / beatDuration)
        ctx.lineWidth = 1 * dpr
        for (let i = firstBeat; i <= lastBeat; i++) {
          const t = i * beatDuration
          const x = center + (t - position) * pps
          const isBar = i % 4 === 0
          ctx.strokeStyle = isBar ? 'rgba(255,255,255,0.12)' : 'rgba(255,255,255,0.04)'
          ctx.beginPath()
          ctx.moveTo(x, 0)
          ctx.lineTo(x, height)
          ctx.stroke()
        }
      }

      song.tracks.forEach((track, trackIndex) => {
        const y = 20 * dpr + trackIndex * trackHeight
        const color = TRACK_COLORS[trackIndex % TRACK_COLORS.length]

        track.clips.forEach((clip) => {
          const clipEnd = song.endPosition !== undefined
            ? Math.min(clip.position + clip.duration, song.endPosition)
            : clip.position + clip.duration
          const clipX = center + (clip.position - position) * pps
          const clipW = (clipEnd - clip.position) * pps
          if (clipW <= 0 || clipX + clipW < 0 || clipX > width) return

          ctx.fillStyle = color
          ctx.globalAlpha = 0.85
          ctx.fillRect(clipX, y, clipW, trackHeight - 2 * dpr)
          ctx.globalAlpha = 1
          ctx.fillStyle = '#fff'
          ctx.font = '11px sans-serif'
          ctx.fillText(clip.name, clipX + 4, y + trackHeight / 2 + 4)
        })
      })

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
