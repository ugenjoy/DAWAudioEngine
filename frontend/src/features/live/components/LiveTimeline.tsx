import { useEffect, useRef } from 'react'
import { Song } from '@/shared/models/song'
import { Loop } from '@/shared/models/loop'
import type { EventRule, PositionTriggerParams } from '@/shared/models/event-rule'
import type { Marker } from '@/shared/models/marker'
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
  loops: Loop[]
  activeLoop: Loop | null
  positionTriggers?: EventRule[]
  markers?: Marker[]
  pixelsPerSecond?: number
}

export function LiveTimeline({
  song,
  positionRef,
  playheadUpdateRef,
  playing,
  loops,
  activeLoop,
  positionTriggers,
  markers,
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

      // Draw loop regions
      loops.forEach((loop) => {
        const isActive = activeLoop?.id === loop.id
        const loopStartX = center + (loop.start - position) * pps
        const loopEndX = center + (loop.end - position) * pps
        const loopW = loopEndX - loopStartX
        if (loopW <= 0 || loopEndX < 0 || loopStartX > width) return

        ctx.globalAlpha = isActive ? 0.25 : 0.12
        ctx.fillStyle = isActive ? '#f59e0b' : '#6366f1'
        ctx.fillRect(loopStartX, 0, loopW, height)

        ctx.globalAlpha = isActive ? 0.9 : 0.5
        ctx.strokeStyle = isActive ? '#f59e0b' : '#6366f1'
        ctx.lineWidth = 2 * dpr
        ctx.beginPath()
        ctx.moveTo(loopStartX, 0)
        ctx.lineTo(loopStartX, height)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(loopEndX, 0)
        ctx.lineTo(loopEndX, height)
        ctx.stroke()
      })
      ctx.globalAlpha = 1

      // Position trigger markers
      if (positionTriggers) {
        for (const trigger of positionTriggers) {
          const tp = trigger.triggerParams as PositionTriggerParams | undefined
          if (tp?.position === undefined) continue
          const x = center + (tp.position - position) * pps
          if (x < 0 || x > width) continue

          ctx.globalAlpha = trigger.enabled ? 1.0 : 0.4
          ctx.strokeStyle = '#a855f7'
          ctx.lineWidth = 2 * dpr
          ctx.beginPath()
          ctx.moveTo(x, 0)
          ctx.lineTo(x, height)
          ctx.stroke()

          ctx.fillStyle = '#a855f7'
          ctx.beginPath()
          ctx.moveTo(x - 5 * dpr, 0)
          ctx.lineTo(x + 5 * dpr, 0)
          ctx.lineTo(x, 6 * dpr)
          ctx.closePath()
          ctx.fill()
          ctx.globalAlpha = 1.0
        }
      }

      // Draw markers (amber flags)
      if (markers) {
        for (const marker of markers) {
          const x = center + (marker.position - position) * pps
          if (x < 0 || x > width) continue

          ctx.strokeStyle = '#f59e0b'
          ctx.lineWidth = 1.5 * dpr
          ctx.beginPath()
          ctx.moveTo(x, 0)
          ctx.lineTo(x, height)
          ctx.stroke()

          // Flag triangle at top
          ctx.fillStyle = '#f59e0b'
          ctx.beginPath()
          ctx.moveTo(x, 0)
          ctx.lineTo(x + 10 * dpr, 0)
          ctx.lineTo(x + 10 * dpr, 8 * dpr)
          ctx.lineTo(x, 8 * dpr)
          ctx.closePath()
          ctx.fill()

          // Label
          ctx.fillStyle = '#000'
          ctx.font = `bold ${9 * dpr}px sans-serif`
          ctx.fillText(marker.name, x + 2 * dpr, 7 * dpr)
        }
      }

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
  }, [song, pixelsPerSecond, loops, activeLoop, positionTriggers, markers])

  return <canvas ref={canvasRef} className="w-full h-full" />
}
