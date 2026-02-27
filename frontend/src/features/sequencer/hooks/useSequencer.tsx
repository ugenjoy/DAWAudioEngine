import { useProject } from '@/shared/contexts/project-provider'
import { useCallback } from 'react'
import { getCSSVar } from '../utils'
import { drawClip } from '../canvas/clips'
import { useInterpolatedPlayhead } from './useInterpolatedPlayhead'

export function useSequencer(zoom: number, scrollX: number, scrollY: number) {
  const { activeSong, playheadPos, cursorPos, trackViews, playing } =
    useProject()
  const interpolatedPlayheadPos = useInterpolatedPlayhead(playheadPos, playing)

  const draw = useCallback(
    (ctx: CanvasRenderingContext2D, width: number, height: number) => {
      if (!activeSong) return
      ctx.clearRect(0, 0, width, height)

      const basePixelsPerBeat = 20
      const pixelsPerBeat = basePixelsPerBeat * zoom

      const headerHeight = 20
      let totalHeight = 0

      // Clips
      for (const [index, trackView] of trackViews.entries()) {
        if (index % 2) {
          ctx.fillStyle = getCSSVar('--track-background-1')
          ctx.fillRect(
            0,
            totalHeight + headerHeight - scrollY,
            width,
            trackView.height,
          )
        } else {
          ctx.fillStyle = getCSSVar('--track-background-2')
          ctx.fillRect(
            0,
            totalHeight + headerHeight - scrollY,
            width,
            trackView.height,
          )
        }

        if (trackView.track.type === 'AudioFileTrack') {
          for (const clip of trackView.track.clips) {
            const offset = 1.5
            const x =
              (clip.position / 60) * activeSong.tempo * pixelsPerBeat - scrollX
            const y = totalHeight + headerHeight + offset - scrollY
            const width =
              (clip.duration / 60) * activeSong.tempo * pixelsPerBeat
            const height = trackView.height - offset * 2
            const waveform =
              clip.type === 'AudioClip' ? clip.waveform : undefined
            drawClip(
              { x, y, width, height },
              trackView.fillColor,
              trackView.strokeColor,
              waveform,
              ctx,
            )
          }
        }
        totalHeight += trackView.height
      }

      const pixelsPerSub = pixelsPerBeat / 4
      const pixelsPerBar = pixelsPerBeat * 4
      const pixelsPer4Bar = pixelsPerBeat * 16

      let pixelsPerLine = pixelsPerBeat

      if (pixelsPerBeat < 4) {
        pixelsPerLine = pixelsPer4Bar
      } else if (pixelsPerBeat < 16 && pixelsPerBeat >= 4) {
        pixelsPerLine = pixelsPerBar
      } else if (pixelsPerBeat > 64) {
        pixelsPerLine = pixelsPerSub
      }

      // Header Rect
      ctx.fillStyle = getCSSVar('--background')
      ctx.fillRect(0, 0, width, headerHeight)

      // Grid
      const firstVisibleLine = Math.floor(scrollX / pixelsPerLine)
      const lastVisibleLine = Math.ceil((scrollX + width) / pixelsPerLine)

      for (let i = firstVisibleLine; i <= lastVisibleLine; i++) {
        const x = i * pixelsPerLine - scrollX

        if (i % 4 === 0) {
          ctx.strokeStyle = getCSSVar('--grid-accent')
          ctx.fillStyle = getCSSVar('--foreground')
          ctx.font = '12px Sans'
        } else {
          ctx.strokeStyle = getCSSVar('--grid')
          if (pixelsPerLine > 40) {
            ctx.fillStyle = getCSSVar('--muted-foreground')
            ctx.font = '10px Sans'
          } else {
            ctx.fillStyle = 'transparent'
          }
        }

        let text = ''
        if (pixelsPerLine === pixelsPerSub) {
          const bar = Math.floor(i / 16) + 1
          const beat = Math.floor((i % 16) / 4) + 1
          const sub = (i % 4) + 1

          if (sub === 1) {
            text = `${bar}.${beat}`
          } else {
            text = `${bar}.${beat}.${sub}`
          }
        } else if (pixelsPerLine === pixelsPerBeat) {
          const bar = Math.floor(i / 4) + 1
          const beat = (i % 4) + 1

          if (beat === 1) {
            text = `${bar}`
          } else {
            text = `${bar}.${beat}`
          }
        } else if (pixelsPerLine === pixelsPerBar) {
          text = `${i + 1}`
        } else if (pixelsPerLine === pixelsPer4Bar) {
          text = `${i * 4 + 1}`
        }

        // Line
        ctx.beginPath()
        ctx.moveTo(x, 0)
        ctx.lineTo(x, height)
        ctx.stroke()
        ctx.lineWidth = 1

        // Marker
        ctx.font = '10px Arial'
        ctx.fillText(text, x + 4, 13)
      }

      // Cursor
      const cursorPosPx =
        (cursorPos / 60) * activeSong.tempo * pixelsPerBeat - scrollX

      ctx.strokeStyle = getCSSVar('--cursor')
      ctx.lineWidth = 0.5
      ctx.beginPath()
      ctx.moveTo(cursorPosPx, 0)
      ctx.lineTo(cursorPosPx, height)
      ctx.stroke()

      ctx.fillStyle = getCSSVar('--primary')
      ctx.lineWidth = 0.5
      ctx.beginPath()
      ctx.moveTo(cursorPosPx - 5, 0)
      ctx.lineTo(cursorPosPx + 5, 0)
      ctx.lineTo(cursorPosPx, 5)
      ctx.fill()

      // Playhead (interpolated for smooth rendering)
      const playheadPosPx =
        (interpolatedPlayheadPos.current / 60) *
          activeSong.tempo *
          pixelsPerBeat -
        scrollX

      ctx.strokeStyle = getCSSVar('--playhead')
      ctx.lineWidth = 1
      ctx.beginPath()
      ctx.moveTo(playheadPosPx, 0)
      ctx.lineTo(playheadPosPx, height)
      ctx.stroke()
    },
    [cursorPos, activeSong, trackViews, zoom, scrollX, scrollY],
  )
  return { draw, playing }
}
