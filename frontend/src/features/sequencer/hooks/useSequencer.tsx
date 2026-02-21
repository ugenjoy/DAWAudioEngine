import { useProject } from '@/shared/contexts/project-provider'
import { useCallback } from 'react'
import { getCSSVar } from '../utils'
import { drawClip } from '../canvas/clips'

export function useSequencer(zoom: number, scrollX: number) {
  const { activeSong, transportPos, trackViews } = useProject()

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
          ctx.fillRect(0, totalHeight + headerHeight, width, trackView.height)
        } else {
          ctx.fillStyle = getCSSVar('--track-background-2')
          ctx.fillRect(0, totalHeight + headerHeight, width, trackView.height)
        }

        if (trackView.track.type === 'AudioFileTrack') {
          for (const clip of trackView.track.clips) {
            const offset = 1.5
            const x =
              (clip.position / 60) * activeSong.tempo * pixelsPerBeat - scrollX
            const y = totalHeight + headerHeight + offset
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

      const firstVisibleLine = Math.floor(scrollX / pixelsPerLine)
      const lastVisibleLine = Math.ceil((scrollX + width) / pixelsPerLine)

      // Grid
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
      const cursorPos =
        (transportPos / 60) * activeSong.tempo * pixelsPerBeat - scrollX

      ctx.strokeStyle = getCSSVar('--foreground')
      ctx.lineWidth = 1
      ctx.beginPath()
      ctx.moveTo(cursorPos, 0)
      ctx.lineTo(cursorPos, height)
      ctx.stroke()
    },
    [transportPos, activeSong, trackViews, zoom, scrollX],
  )
  return { draw }
}
