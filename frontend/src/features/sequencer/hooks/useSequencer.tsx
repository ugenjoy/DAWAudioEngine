import { useProject, type TrackView } from '@/shared/contexts/project-provider'
import { useCallback, type RefObject } from 'react'
import { getCSSVar } from '../utils'
import { drawClip } from '../canvas/clips'
import { useInterpolatedPlayhead } from './useInterpolatedPlayhead'

export interface GhostClip {
  position: number
  trackIndex: number
}

export interface SelectedClip {
  trackId: string
  clipId: string
}

export interface DraggingClip {
  trackId: string
  clipId: string
  position: number
}

export function useSequencer(
  zoomRef: RefObject<number>,
  scrollXRef: RefObject<number>,
  scrollYRef: RefObject<number>,
  trackViewsOverride?: TrackView[],
  selectedTrackId?: string | null,
  ghostClipRef?: RefObject<GhostClip | null>,
  selectedClip?: SelectedClip | null,
  draggingClipRef?: RefObject<DraggingClip | null>,
) {
  const {
    activeSong,
    playheadPosRef,
    playheadUpdateRef,
    cursorPosRef,
    trackViews: contextTrackViews,
    playing,
  } = useProject()
  const trackViews = trackViewsOverride ?? contextTrackViews
  const interpolatedPlayheadPos = useInterpolatedPlayhead(playheadPosRef, playheadUpdateRef, playing)

  const draw = useCallback(
    (ctx: CanvasRenderingContext2D, width: number, height: number) => {
      if (!activeSong) return

      // Read refs once at the start of each frame
      const zoom = zoomRef.current
      const scrollX = scrollXRef.current
      const scrollY = scrollYRef.current

      ctx.clearRect(0, 0, width, height)

      const basePixelsPerBeat = 20
      const pixelsPerBeat = basePixelsPerBeat * zoom

      const headerHeight = 22
      let totalHeight = 0

      // Tracks & clips with viewport culling
      for (const [index, trackView] of trackViews.entries()) {
        const trackY = totalHeight + headerHeight - scrollY

        // Skip tracks fully off-screen vertically
        if (trackY + trackView.height < 0 || trackY > height) {
          totalHeight += trackView.height
          continue
        }

        const isSelected = trackView.track.id === selectedTrackId

        if (index % 2) {
          ctx.fillStyle = getCSSVar('--track-background-1')
        } else {
          ctx.fillStyle = getCSSVar('--track-background-2')
        }
        ctx.fillRect(0, trackY, width, trackView.height)

        if (isSelected) {
          ctx.fillStyle = 'rgba(255, 255, 255, 0.04)'
          ctx.fillRect(0, trackY, width, trackView.height)
        }

        if (trackView.track.type === 'AudioFileTrack') {
          if (isSelected) ctx.globalAlpha = 1
          else ctx.globalAlpha = 0.7

          for (const clip of trackView.track.clips) {
            const isClipSelected =
              selectedClip?.trackId === trackView.track.id &&
              selectedClip?.clipId === clip.id
            const isDragging =
              draggingClipRef?.current?.clipId === clip.id &&
              draggingClipRef?.current?.trackId === trackView.track.id

            const clipPosition = isDragging
              ? draggingClipRef!.current!.position
              : clip.position

            const clipOffset = 1.5
            const x =
              (clipPosition / 60) * activeSong.tempo * pixelsPerBeat - scrollX
            const w =
              (clip.duration / 60) * activeSong.tempo * pixelsPerBeat

            // Skip clips fully off-screen horizontally
            if (x + w < 0 || x > width) continue

            const y = totalHeight + headerHeight + clipOffset - scrollY
            const h = trackView.height - clipOffset * 2
            const waveform =
              clip.type === 'AudioClip' ? clip.waveform : undefined

            if (isDragging) ctx.globalAlpha = 0.5

            drawClip(
              { x, y, width: w, height: h },
              trackView.fillColor,
              trackView.strokeColor,
              waveform,
              ctx,
              isClipSelected,
            )

            if (isDragging) {
              ctx.globalAlpha = isSelected ? 1 : 0.7
            }
          }
          ctx.globalAlpha = 1
        }
        totalHeight += trackView.height
      }

      // Ghost clip preview during file drag
      const ghost = ghostClipRef?.current
      if (ghost && ghost.trackIndex >= 0 && ghost.trackIndex < trackViews.length) {
        const tv = trackViews[ghost.trackIndex]
        let ghostY = headerHeight - scrollY
        for (let i = 0; i < ghost.trackIndex; i++) {
          ghostY += trackViews[i].height
        }
        const offset = 1.5
        const ghostX =
          (ghost.position / 60) * activeSong.tempo * pixelsPerBeat - scrollX
        const ghostWidth = 4 * pixelsPerBeat
        const ghostHeight = tv.height - offset * 2

        ctx.globalAlpha = 0.4
        ctx.fillStyle = getCSSVar(tv.fillColor)
        ctx.fillRect(ghostX, ghostY + offset, ghostWidth, ghostHeight)
        ctx.strokeStyle = getCSSVar(tv.strokeColor)
        ctx.setLineDash([4, 4])
        ctx.strokeRect(ghostX, ghostY + offset, ghostWidth, ghostHeight)
        ctx.setLineDash([])
        ctx.globalAlpha = 1
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

        ctx.beginPath()
        ctx.moveTo(x, 0)
        ctx.lineTo(x, height)
        ctx.stroke()
        ctx.lineWidth = 1

        ctx.font = '10px Arial'
        ctx.fillText(text, x + 4, 13)
      }

      // Cursor
      const cursorPos = cursorPosRef.current
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
    [activeSong, trackViews, selectedTrackId, selectedClip],
  )
  return { draw }
}
