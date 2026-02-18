import { useProject } from '@/shared/contexts/project-provider'
import { useCallback } from 'react'
import { getCSSVar } from '../utils'
import { drawClip } from '../canvas/clips'

export function useSequencer(zoom: number, scrollX: number) {
  const { activeSong, transportPos } = useProject()

  const draw = useCallback(
    (ctx: CanvasRenderingContext2D, width: number, height: number) => {
      if (!activeSong) return
      ctx.clearRect(0, 0, width, height)

      const basePixelsPerBeat = 20
      const pixelsPerBeat = basePixelsPerBeat * zoom

      // Clips
      for (const [index, track] of activeSong.tracks.entries()) {
        if (index % 2) {
          ctx.fillStyle = getCSSVar('--track-background-1')
          ctx.fillRect(0, index * 80, width, 80)
        } else {
          ctx.fillStyle = getCSSVar('--track-background-2')
          ctx.fillRect(0, index * 80, width, 80)
        }

        if (track.type === 'AudioFileTrack') {
          for (const clip of track.clips) {
            const offset = 1.5
            const x = (clip.position / 60) * activeSong.tempo * pixelsPerBeat - scrollX
            const y = index * 80 + offset
            const width =
              (clip.duration / 60) * activeSong.tempo * pixelsPerBeat
            const height = 80 - offset * 2

            const waveform =
              clip.type === 'AudioClip' ? clip.waveform : undefined
            drawClip({ x, y, width, height }, waveform, ctx)
          }
        }
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
          ctx.fillStyle = getCSSVar('--muted-foreground')
          ctx.font = '10px Sans'
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
        ctx.fillText(text, x + 4, 10)
      }

      // Cursor
      const cursorPos = (transportPos / 60) * activeSong.tempo * pixelsPerBeat - scrollX

      ctx.strokeStyle = getCSSVar('--foreground')
      ctx.lineWidth = 1
      ctx.beginPath()
      ctx.moveTo(cursorPos, 0)
      ctx.lineTo(cursorPos, height)
      ctx.stroke()
    },
    [transportPos, activeSong, zoom, scrollX],
  )
  return { draw }
}
