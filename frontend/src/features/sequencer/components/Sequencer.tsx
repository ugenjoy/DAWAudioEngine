import { useProject } from '@/shared/contexts/project-provider'
import { Transport } from '../../transport/components/Transport'
import Track from './Track'
import {
  forwardRef,
  memo,
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react'
import Timeline from './Timeline'
import {
  useSequencer,
  type GhostClip,
  type SelectedClip,
  type DraggingClip,
} from '../hooks/useSequencer'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { useMode } from '@/shared/contexts/mode-provider'
import { useEvents } from '@/shared/contexts/events-provider'
import { useMarkers } from '@/shared/contexts/markers-provider'
import type { Marker } from '@/shared/models/marker'
import {
  DndContext,
  closestCenter,
  PointerSensor,
  useSensor,
  useSensors,
  type DragEndEvent,
  type DragOverEvent,
} from '@dnd-kit/core'
import {
  SortableContext,
  useSortable,
  verticalListSortingStrategy,
  arrayMove,
} from '@dnd-kit/sortable'
import { CSS } from '@dnd-kit/utilities'
import { Button } from '@/shared/shadcn/components/button'
import { IconPlus } from '@tabler/icons-react'
import { uploadAudioClip } from '@/shared/services/audio-upload'
import type { TrackView } from '@/shared/contexts/project-provider'
import type { AudioInput } from '@/shared/models/audio-input'
import {
  ContextMenu,
  ContextMenuContent,
  ContextMenuItem,
  ContextMenuTrigger,
} from '@/shared/shadcn/components/context-menu'
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
} from '@/shared/shadcn/components/alert-dialog'

type SortableTrackProps = {
  trackView: TrackView
  availableInputs: AudioInput[]
  trackLevelsRef: React.RefObject<Record<string, number>>
  selected: boolean
  sortDisabled: boolean
  onTrackSelect: (trackId: string) => void
  onSetInput: (trackId: string, inputChannel: number, stereo: boolean) => void
  onSetMonitoring: (trackId: string, monitoring: boolean) => void
  onSetMute: (trackId: string, mute: boolean) => void
  onSetSolo: (trackId: string, solo: boolean) => void
  onSetVolume: (trackId: string, volume: number) => void
  onRename: (trackId: string, name: string) => void
  onSetColor: (trackId: string, color: number) => void
  onResize: (trackId: string, height: number) => void
}

const SortableTrack = memo(
  forwardRef<
    HTMLDivElement,
    SortableTrackProps & React.HTMLAttributes<HTMLDivElement>
  >(function SortableTrack(
    {
      trackView,
      availableInputs,
      trackLevelsRef,
      selected,
      sortDisabled,
      onTrackSelect,
      onSetInput,
      onSetMonitoring,
      onSetMute,
      onSetSolo,
      onSetVolume,
      onRename,
      onSetColor,
      onResize,
      ...restProps
    },
    externalRef,
  ) {
    const { attributes, listeners, setNodeRef, transform, transition } =
      useSortable({ id: trackView.track.id, disabled: sortDisabled })

    const restrictedTransform = transform
      ? { ...transform, x: 0, scaleX: 1, scaleY: 1 }
      : transform
    const style = {
      transform: CSS.Transform.toString(restrictedTransform),
      transition,
    }

    const mergedRef = (node: HTMLDivElement | null) => {
      setNodeRef(node)
      if (typeof externalRef === 'function') externalRef(node)
      else if (externalRef) externalRef.current = node
    }

    return (
      <Track
        ref={mergedRef}
        {...restProps}
        style={{ ...style, ...restProps.style }}
        {...attributes}
        {...listeners}
        id={trackView.track.id}
        name={trackView.track.name}
        mute={trackView.track.mute}
        solo={trackView.track.solo}
        volume={trackView.track.volume}
        color={trackView.strokeColor}
        height={trackView.height}
        inputChannel={trackView.track.inputChannel ?? -1}
        inputStereo={trackView.track.inputStereo ?? false}
        monitoring={trackView.track.monitoring ?? false}
        availableInputs={availableInputs}
        trackLevelsRef={trackLevelsRef}
        selected={selected}
        onTrackSelect={onTrackSelect}
        onSetInput={onSetInput}
        onSetMonitoring={onSetMonitoring}
        onSetMute={onSetMute}
        onSetSolo={onSetSolo}
        onSetVolume={onSetVolume}
        onRename={onRename}
        onSetColor={onSetColor}
        onResize={onResize}
      />
    )
  }),
)

type LoopInteraction =
  | { mode: 'creating'; start: number; end: number }
  | {
      mode: 'resizing'
      loopId: string
      edge: 'start' | 'end'
      origStart: number
      origEnd: number
    }
  | {
      mode: 'moving'
      loopId: string
      origStart: number
      origEnd: number
      dragStartX: number
    }

function Sequencer() {
  const {
    project,
    playing,
    activeSong,
    cursorPosRef,
    trackViews,
    availableInputs,
    setTrackInput,
    setTrackMonitoring,
    setTrackMute,
    setTrackSolo,
    setTrackColor,
    setTrackHeight,
    setTrackVolume,
    addTrack,
    removeTrack,
    renameTrack,
    reorderTrack,
    trackLevelsRef,
    songLoading,
    addLoop,
    removeLoop,
    updateLoop,
    loops,
  } = useProject()
  const { isLiveMode } = useMode()
  const { songEvents } = useEvents()
  const { markers, addMarker, removeMarker, updateMarker } = useMarkers()
  const positionTriggers = useMemo(
    () => songEvents.filter((r) => r.trigger === 'position'),
    [songEvents],
  )
  const { send } = useWebSocket()

  const HEADER_HEIGHT = 22

  // Canvas-only values as refs — no React re-renders on zoom/scroll
  const zoomRef = useRef(1)
  const scrollXRef = useRef(0)
  const scrollYRef = useRef(0)

  const [selectedTrackId, setSelectedTrackId] = useState<string | null>(null)
  const [deleteDialogOpen, setDeleteDialogOpen] = useState(false)
  const ghostClipRef = useRef<GhostClip | null>(null)
  const [selectedClip, setSelectedClip] = useState<SelectedClip | null>(null)
  const draggingClipRef = useRef<DraggingClip | null>(null)
  const [isDraggingClip, setIsDraggingClip] = useState(false)
  const dragStartRef = useRef<{ x: number; originPos: number } | null>(null)
  const endPositionDragRef = useRef<number | null>(null)
  const isDraggingEndPosition = useRef(false)
  const draggingMarkerRef = useRef<{ markerId: string; position: number } | null>(null)
  const loopInteractionRef = useRef<LoopInteraction | null>(null)
  const loopPreviewRef = useRef<{
    loopId: string | null
    start: number
    end: number
  } | null>(null)
  const [selectedLoopId, setSelectedLoopId] = useState<string | null>(null)
  const [headerCursor, setHeaderCursor] = useState<string>('default')
  const [clipContextMenu, setClipContextMenu] = useState<{
    x: number
    y: number
  } | null>(null)
  const [endPositionContextMenu, setEndPositionContextMenu] = useState<{
    x: number
    y: number
    position: number
  } | null>(null)
  const [markerContextMenu, setMarkerContextMenu] = useState<{
    x: number
    y: number
    position: number
    existingMarker?: Marker
  } | null>(null)
  const [dragTrackViews, setDragTrackViews] = useState<
    typeof trackViews | null
  >(null)
  const activeTrackViews = dragTrackViews ?? trackViews
  const { draw } = useSequencer(
    zoomRef,
    scrollXRef,
    scrollYRef,
    activeTrackViews,
    selectedTrackId,
    ghostClipRef,
    selectedClip,
    draggingClipRef,
    endPositionDragRef,
    loopPreviewRef,
    selectedLoopId,
    positionTriggers,
    draggingMarkerRef,
    markers,
  )

  const tracksContainer = useRef<HTMLDivElement>(null)
  const isProgrammaticScroll = useRef(false)

  const targetScrollX = useRef(0)
  const targetScrollY = useRef(0)
  const currentScrollX = useRef(0)
  const currentScrollY = useRef(0)
  const animFrameId = useRef(0)

  const syncScrollY = useCallback((value: number) => {
    scrollYRef.current = value
    if (tracksContainer.current) {
      isProgrammaticScroll.current = true
      tracksContainer.current.scrollTo({ top: value, behavior: 'instant' })
    }
  }, [])

  const animateScroll = useCallback(() => {
    const lerpFactor = 0.15
    const threshold = 0.5
    let needsUpdate = false

    const diffX = targetScrollX.current - currentScrollX.current
    if (Math.abs(diffX) < threshold) {
      currentScrollX.current = targetScrollX.current
    } else {
      currentScrollX.current += diffX * lerpFactor
      needsUpdate = true
    }

    const diffY = targetScrollY.current - currentScrollY.current
    if (Math.abs(diffY) < threshold) {
      currentScrollY.current = targetScrollY.current
    } else {
      currentScrollY.current += diffY * lerpFactor
      needsUpdate = true
    }

    scrollXRef.current = currentScrollX.current
    syncScrollY(currentScrollY.current)

    if (needsUpdate) {
      animFrameId.current = requestAnimationFrame(animateScroll)
    } else {
      animFrameId.current = 0
    }
  }, [syncScrollY])

  const startScrollAnimation = useCallback(() => {
    if (!animFrameId.current) {
      animFrameId.current = requestAnimationFrame(animateScroll)
    }
  }, [animateScroll])

  useEffect(() => {
    return () => {
      if (animFrameId.current) cancelAnimationFrame(animFrameId.current)
    }
  }, [])

  const handleWheel = useCallback(
    (e: WheelEvent) => {
      e.preventDefault()

      const linePx = 20
      const pagePx = 400
      const modeMultiplier =
        e.deltaMode === 1 ? linePx : e.deltaMode === 2 ? pagePx : 1
      const scrollSpeed = 0.5

      if (e.ctrlKey) {
        const zoomFactor = e.deltaY > 0 ? 0.9 : 1.1
        const canvasWidth = (e.target as HTMLElement).getBoundingClientRect()
          .width
        const prevZoom = zoomRef.current
        const newZoom = Math.max(0.02, Math.min(100, prevZoom * zoomFactor))
        zoomRef.current = newZoom

        const cursorPosRawPx =
          (cursorPosRef.current / 60) * activeSong!.tempo * 20
        const cursorPosScreenX = cursorPosRawPx * prevZoom - scrollXRef.current
        const offset = 100
        const anchorScreenX = Math.max(
          offset,
          Math.min(canvasWidth - offset, cursorPosScreenX),
        )
        const newScrollX = Math.max(0, cursorPosRawPx * newZoom - anchorScreenX)
        scrollXRef.current = newScrollX
        targetScrollX.current = newScrollX
        currentScrollX.current = newScrollX
      } else {
        const rawDeltaX = e.deltaX * modeMultiplier
        const rawDeltaY = e.deltaY * modeMultiplier
        const deltaX = e.shiftKey ? rawDeltaY : rawDeltaX
        const deltaY = e.shiftKey ? 0 : rawDeltaY

        if (deltaX !== 0) {
          targetScrollX.current = Math.max(
            0,
            targetScrollX.current + deltaX * scrollSpeed,
          )
        }

        if (deltaY !== 0) {
          const maxScrollY = tracksContainer.current
            ? tracksContainer.current.scrollHeight -
              tracksContainer.current.clientHeight
            : 0
          targetScrollY.current = Math.max(
            0,
            Math.min(maxScrollY, targetScrollY.current + deltaY * scrollSpeed),
          )
        }

        startScrollAnimation()
      }
    },
    [activeSong, startScrollAnimation],
  )

  const hitTestClip = useCallback(
    (
      offsetX: number,
      offsetY: number,
    ): { trackId: string; clipId: string; clipPosition: number } | null => {
      if (!activeSong) return null
      const pixelsPerBeat = 20 * zoomRef.current
      const headerHeight = 20
      const y = offsetY + scrollYRef.current - headerHeight
      if (y < 0) return null

      let accHeight = 0
      for (const tv of activeTrackViews) {
        if (
          y >= accHeight &&
          y < accHeight + tv.height &&
          tv.track.type === 'AudioFileTrack'
        ) {
          for (const clip of tv.track.clips) {
            const clipX =
              (clip.position / 60) * activeSong.tempo * pixelsPerBeat -
              scrollXRef.current
            const clipW =
              (clip.duration / 60) * activeSong.tempo * pixelsPerBeat
            if (offsetX >= clipX && offsetX <= clipX + clipW) {
              return {
                trackId: tv.track.id,
                clipId: clip.id,
                clipPosition: clip.position,
              }
            }
          }
          break
        }
        accHeight += tv.height
      }
      return null
    },
    [activeSong, activeTrackViews],
  )

  const hitTestMarker = useCallback(
    (offsetX: number): Marker | undefined => {
      if (!activeSong) return undefined
      const pixelsPerBeat = 20 * zoomRef.current
      return markers.find((m) => {
        const x = (m.position / 60) * activeSong.tempo * pixelsPerBeat - scrollXRef.current
        return Math.abs(offsetX - x) < 8
      })
    },
    [activeSong, markers],
  )

  const hitTestLoop = useCallback(
    (
      offsetX: number,
    ): {
      loopId: string
      zone: 'start-edge' | 'end-edge' | 'interior'
    } | null => {
      if (!activeSong) return null
      const pixelsPerBeat = 20 * zoomRef.current
      const EDGE_HIT_ZONE = 8
      for (const loop of loops) {
        const startPx =
          (loop.start / 60) * activeSong.tempo * pixelsPerBeat -
          scrollXRef.current
        const endPx =
          (loop.end / 60) * activeSong.tempo * pixelsPerBeat -
          scrollXRef.current
        if (Math.abs(offsetX - startPx) <= EDGE_HIT_ZONE)
          return { loopId: loop.id, zone: 'start-edge' }
        if (Math.abs(offsetX - endPx) <= EDGE_HIT_ZONE)
          return { loopId: loop.id, zone: 'end-edge' }
        if (offsetX > startPx && offsetX < endPx)
          return { loopId: loop.id, zone: 'interior' }
      }
      return null
    },
    [activeSong, loops],
  )

  const snapPosition = useCallback(
    (offsetX: number): number => {
      if (!activeSong) return 0
      const zoom = zoomRef.current
      const pixelsPerBeat = 20 * zoom
      let beatsPerLine: number
      if (pixelsPerBeat < 4) beatsPerLine = 16
      else if (pixelsPerBeat < 16) beatsPerLine = 4
      else if (pixelsPerBeat > 64) beatsPerLine = 0.25
      else beatsPerLine = 1
      const rawPos =
        ((offsetX + scrollXRef.current) * 60) / (activeSong.tempo * 20 * zoom)
      const snapInterval = (beatsPerLine * 60) / activeSong.tempo
      return Math.max(0, Math.round(rawPos / snapInterval) * snapInterval)
    },
    [activeSong],
  )

  const handleClick = useCallback(
    (e: MouseEvent) => {
      if (!activeSong) return
      if (e.button === 2) return
      e.preventDefault()

      if (e.offsetY < HEADER_HEIGHT) {
        // 1. Ctrl held → start creating a loop
        if (e.ctrlKey || e.metaKey) {
          const snapped = snapPosition(e.offsetX)
          loopInteractionRef.current = {
            mode: 'creating',
            start: snapped,
            end: snapped,
          }
          loopPreviewRef.current = {
            loopId: null,
            start: snapped,
            end: snapped,
          }
          setHeaderCursor('crosshair')
          return
        }

        // 2. End position handle
        if (activeSong.endPosition !== undefined) {
          const pixelsPerBeat = 20 * zoomRef.current
          const endPosPx =
            (activeSong.endPosition / 60) * activeSong.tempo * pixelsPerBeat -
            scrollXRef.current
          if (Math.abs(e.offsetX - endPosPx) < 8) {
            isDraggingEndPosition.current = true
            endPositionDragRef.current = activeSong.endPosition
            return
          }
        }

        // 3. Marker drag
        const markerHit = hitTestMarker(e.offsetX)
        if (markerHit) {
          draggingMarkerRef.current = {
            markerId: markerHit.id,
            position: markerHit.position,
          }
          return
        }

        // 4. Loop hit test
        const loopHit = hitTestLoop(e.offsetX)
        if (loopHit) {
          const loop = loops.find((l) => l.id === loopHit.loopId)
          if (!loop) return
          if (loopHit.zone === 'start-edge') {
            loopInteractionRef.current = {
              mode: 'resizing',
              loopId: loopHit.loopId,
              edge: 'start',
              origStart: loop.start,
              origEnd: loop.end,
            }
            loopPreviewRef.current = {
              loopId: loopHit.loopId,
              start: loop.start,
              end: loop.end,
            }
            setHeaderCursor('ew-resize')
          } else if (loopHit.zone === 'end-edge') {
            loopInteractionRef.current = {
              mode: 'resizing',
              loopId: loopHit.loopId,
              edge: 'end',
              origStart: loop.start,
              origEnd: loop.end,
            }
            loopPreviewRef.current = {
              loopId: loopHit.loopId,
              start: loop.start,
              end: loop.end,
            }
            setHeaderCursor('ew-resize')
          } else {
            setSelectedLoopId(loopHit.loopId)
            loopInteractionRef.current = {
              mode: 'moving',
              loopId: loopHit.loopId,
              origStart: loop.start,
              origEnd: loop.end,
              dragStartX: e.offsetX,
            }
            loopPreviewRef.current = {
              loopId: loopHit.loopId,
              start: loop.start,
              end: loop.end,
            }
            setHeaderCursor('grabbing')
          }
          return
        }

        // 4. Empty header → deselect loop, set playhead cursor
        setSelectedLoopId(null)
        const zoom = zoomRef.current
        const pixelsPerBeat = 20 * zoom
        let beatsPerLine: number
        if (pixelsPerBeat < 4) beatsPerLine = 16
        else if (pixelsPerBeat < 16) beatsPerLine = 4
        else if (pixelsPerBeat > 64) beatsPerLine = 0.25
        else beatsPerLine = 1
        const rawPos =
          ((e.offsetX + scrollXRef.current) * 60) /
          (activeSong.tempo * 20 * zoom)
        const snapInterval = (beatsPerLine * 60) / activeSong.tempo
        const cursorPos = Math.round(rawPos / snapInterval) * snapInterval
        send({ action: 'transport.setCursorPosition', position: cursorPos })
        return
      }

      // Below header: clip and track selection — always deselect loop
      setSelectedLoopId(null)
      const hit = hitTestClip(e.offsetX, e.offsetY)
      if (hit && !isLiveMode) {
        setSelectedClip({ trackId: hit.trackId, clipId: hit.clipId })
        dragStartRef.current = { x: e.offsetX, originPos: hit.clipPosition }
        setSelectedTrackId(hit.trackId)
        return
      }

      setSelectedClip(null)

      const zoom = zoomRef.current
      const pixelsPerBeat = 20 * zoom
      let beatsPerLine: number
      if (pixelsPerBeat < 4) beatsPerLine = 16
      else if (pixelsPerBeat < 16) beatsPerLine = 4
      else if (pixelsPerBeat > 64) beatsPerLine = 0.25
      else beatsPerLine = 1
      const rawPos =
        ((e.offsetX + scrollXRef.current) * 60) / (activeSong.tempo * 20 * zoom)
      const snapInterval = (beatsPerLine * 60) / activeSong.tempo
      const cursorPos = Math.round(rawPos / snapInterval) * snapInterval
      send({ action: 'transport.setCursorPosition', position: cursorPos })

      const clickY = e.offsetY + scrollYRef.current - HEADER_HEIGHT
      if (clickY >= 0) {
        let accHeight = 0
        for (const tv of activeTrackViews) {
          accHeight += tv.height
          if (clickY < accHeight) {
            setSelectedTrackId(tv.track.id)
            break
          }
        }
      }
    },
    [
      activeSong,
      activeTrackViews,
      hitTestClip,
      hitTestLoop,
      hitTestMarker,
      isLiveMode,
      loops,
      send,
      snapPosition,
    ],
  )

  const handleMouseMove = useCallback(
    (e: MouseEvent) => {
      if (!activeSong) return
      if (isLiveMode) return

      const interaction = loopInteractionRef.current

      // Active loop interaction — update preview
      if (interaction) {
        const snapped = snapPosition(e.offsetX)

        if (interaction.mode === 'creating') {
          interaction.end = snapped
          loopPreviewRef.current = {
            loopId: null,
            start: interaction.start,
            end: snapped,
          }
          return
        }

        if (interaction.mode === 'resizing') {
          const newStart =
            interaction.edge === 'start' ? snapped : interaction.origStart
          const newEnd =
            interaction.edge === 'end' ? snapped : interaction.origEnd
          loopPreviewRef.current = {
            loopId: interaction.loopId,
            start: newStart,
            end: newEnd,
          }
          return
        }

        if (interaction.mode === 'moving') {
          const delta =
            snapPosition(e.offsetX) - snapPosition(interaction.dragStartX)
          const newStart = Math.max(0, interaction.origStart + delta)
          const duration = interaction.origEnd - interaction.origStart
          loopPreviewRef.current = {
            loopId: interaction.loopId,
            start: newStart,
            end: newStart + duration,
          }
          return
        }
      }

      // Existing end position drag
      if (isDraggingEndPosition.current) {
        endPositionDragRef.current = snapPosition(e.offsetX)
        return
      }

      // Existing marker drag
      if (draggingMarkerRef.current) {
        draggingMarkerRef.current = {
          ...draggingMarkerRef.current,
          position: snapPosition(e.offsetX),
        }
        return
      }

      // Cursor hover feedback in header
      if (e.offsetY < HEADER_HEIGHT) {
        if (e.ctrlKey) {
          setHeaderCursor('crosshair')
          return
        }
        const hit = hitTestLoop(e.offsetX)
        if (hit?.zone === 'start-edge' || hit?.zone === 'end-edge') {
          setHeaderCursor('ew-resize')
        } else if (hit?.zone === 'interior') {
          setHeaderCursor('grab')
        } else {
          setHeaderCursor('default')
        }
        return
      }

      setHeaderCursor('default')

      // Existing clip drag
      if (!selectedClip || !dragStartRef.current) return
      const dx = Math.abs(e.offsetX - dragStartRef.current.x)
      if (dx < 3 && !isDraggingClip) return
      const zoom = zoomRef.current
      const pixelsPerBeat = 20 * zoom
      const deltaPx = e.offsetX - dragStartRef.current.x
      const deltaSeconds = (deltaPx * 60) / (activeSong.tempo * pixelsPerBeat)
      const rawPos = dragStartRef.current.originPos + deltaSeconds
      let beatsPerLine: number
      if (pixelsPerBeat < 4) beatsPerLine = 16
      else if (pixelsPerBeat < 16) beatsPerLine = 4
      else if (pixelsPerBeat > 64) beatsPerLine = 0.25
      else beatsPerLine = 1
      const snapInterval = (beatsPerLine * 60) / activeSong.tempo
      const snappedPos = Math.max(
        0,
        Math.round(rawPos / snapInterval) * snapInterval,
      )
      draggingClipRef.current = {
        trackId: selectedClip.trackId,
        clipId: selectedClip.clipId,
        position: snappedPos,
      }
      setIsDraggingClip(true)
    },
    [
      activeSong,
      selectedClip,
      isLiveMode,
      isDraggingClip,
      snapPosition,
      hitTestLoop,
    ],
  )

  const handleMouseUp = useCallback(() => {
    const interaction = loopInteractionRef.current

    if (interaction) {
      const preview = loopPreviewRef.current

      if (interaction.mode === 'creating' && preview?.loopId === null) {
        const start = Math.min(preview.start, preview.end)
        const end = Math.max(preview.start, preview.end)
        if (end - start > 0.1) {
          addLoop(start, end)
        }
      } else if (
        interaction.mode === 'resizing' &&
        preview?.loopId === interaction.loopId
      ) {
        const newStart = Math.min(preview.start, preview.end)
        const newEnd = Math.max(preview.start, preview.end)
        if (newEnd - newStart > 0.1) {
          updateLoop(interaction.loopId, newStart, newEnd)
        }
      } else if (
        interaction.mode === 'moving' &&
        preview?.loopId === interaction.loopId
      ) {
        updateLoop(interaction.loopId, preview.start, preview.end)
      }

      loopInteractionRef.current = null
      loopPreviewRef.current = null
      setHeaderCursor('default')
      return
    }

    if (isDraggingEndPosition.current) {
      if (endPositionDragRef.current !== null && activeSong) {
        send({
          action: 'song.setEndPosition',
          songId: activeSong.id,
          endPosition: endPositionDragRef.current,
        })
      }
      endPositionDragRef.current = null
      isDraggingEndPosition.current = false
      return
    }

    if (draggingMarkerRef.current) {
      const { markerId, position } = draggingMarkerRef.current
      updateMarker(markerId, { position })
      draggingMarkerRef.current = null
      return
    }

    if (draggingClipRef.current && isDraggingClip) {
      send({
        action: 'clip.move',
        trackId: draggingClipRef.current.trackId,
        clipId: draggingClipRef.current.clipId,
        position: draggingClipRef.current.position,
      })
    }
    draggingClipRef.current = null
    dragStartRef.current = null
    setIsDraggingClip(false)
  }, [activeSong, isDraggingClip, send, addLoop, updateLoop])

  const handleContextMenu = useCallback(
    (e: MouseEvent) => {
      if (!activeSong || isLiveMode) return

      if (e.offsetY < HEADER_HEIGHT) {
        e.preventDefault()

        // Priority 1: near end position handle
        if (activeSong.endPosition !== undefined) {
          const pixelsPerBeat = 20 * zoomRef.current
          const endPosPx =
            (activeSong.endPosition / 60) * activeSong.tempo * pixelsPerBeat -
            scrollXRef.current
          if (Math.abs(e.offsetX - endPosPx) < 8) {
            setEndPositionContextMenu({
              x: e.clientX,
              y: e.clientY,
              position: snapPosition(e.offsetX),
            })
            return
          }
        }

        // Priority 2+3: marker (existing or empty spot)
        setMarkerContextMenu({
          x: e.clientX,
          y: e.clientY,
          position: snapPosition(e.offsetX),
          existingMarker: hitTestMarker(e.offsetX),
        })
        return
      }

      const hit = hitTestClip(e.offsetX, e.offsetY)
      if (hit) {
        e.preventDefault()
        setSelectedClip({ trackId: hit.trackId, clipId: hit.clipId })
        setClipContextMenu({ x: e.clientX, y: e.clientY })
      }
    },
    [activeSong, isLiveMode, hitTestClip, hitTestMarker, snapPosition],
  )

  const removeSelectedClip = useCallback(() => {
    if (!selectedClip) return
    send({
      action: 'clip.remove',
      trackId: selectedClip.trackId,
      clipId: selectedClip.clipId,
    })
    setSelectedClip(null)
  }, [selectedClip, send])

  const handleKeyDown = useCallback(
    (e: KeyboardEvent) => {
      if (!activeSong) return
      const tag = (e.target as HTMLElement)?.tagName
      if (tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT') return

      switch (e.key) {
        case ' ': {
          e.preventDefault()
          send({ action: `transport.${playing ? 'stop' : 'play'}` })
          break
        }
        case 'Delete': {
          e.preventDefault()
          if (!isLiveMode && selectedLoopId) {
            removeLoop(selectedLoopId)
            setSelectedLoopId(null)
          } else if (!isLiveMode && selectedClip) {
            removeSelectedClip()
          } else if (!isLiveMode && selectedTrackId) {
            setDeleteDialogOpen(true)
          }
          break
        }
      }
    },
    [
      activeSong,
      playing,
      isLiveMode,
      selectedLoopId,
      selectedTrackId,
      selectedClip,
      removeSelectedClip,
      removeLoop,
      send,
    ],
  )

  const confirmDelete = useCallback(() => {
    if (selectedTrackId) {
      removeTrack(selectedTrackId)
      setSelectedTrackId(null)
    }
    setDeleteDialogOpen(false)
  }, [selectedTrackId, removeTrack])

  const AUDIO_EXTENSIONS = ['.wav', '.mp3', '.flac', '.aiff', '.ogg', '.aif']

  const trackIndexFromY = useCallback(
    (offsetY: number): number => {
      const headerHeight = 20
      const y = offsetY + scrollYRef.current - headerHeight
      if (y < 0) return 0
      let accHeight = 0
      for (let i = 0; i < activeTrackViews.length; i++) {
        accHeight += activeTrackViews[i].height
        if (y < accHeight) return i
      }
      return activeTrackViews.length - 1
    },
    [activeTrackViews],
  )

  const handleFileDragOver = useCallback(
    (e: React.DragEvent) => {
      if (isLiveMode || !e.dataTransfer.types.includes('Files')) return
      e.preventDefault()
      e.dataTransfer.dropEffect = 'copy'

      const rect = e.currentTarget.getBoundingClientRect()
      const offsetX = e.clientX - rect.left
      const offsetY = e.clientY - rect.top
      ghostClipRef.current = {
        position: snapPosition(offsetX),
        trackIndex: trackIndexFromY(offsetY),
      }
    },
    [isLiveMode, snapPosition, trackIndexFromY],
  )

  const handleFileDragLeave = useCallback((e: React.DragEvent) => {
    if (e.currentTarget.contains(e.relatedTarget as Node)) return
    ghostClipRef.current = null
  }, [])

  const handleFileDrop = useCallback(
    async (e: React.DragEvent) => {
      e.preventDefault()
      ghostClipRef.current = null

      if (!activeSong || isLiveMode) return

      const files = Array.from(e.dataTransfer.files).filter((f) =>
        AUDIO_EXTENSIONS.some((ext) => f.name.toLowerCase().endsWith(ext)),
      )
      if (files.length === 0) return

      const rect = e.currentTarget.getBoundingClientRect()
      const offsetX = e.clientX - rect.left
      const offsetY = e.clientY - rect.top
      const dropPosition = snapPosition(offsetX)
      const tIndex = trackIndexFromY(offsetY)
      const targetTrackId = activeTrackViews[tIndex]?.track.id
      if (!targetTrackId) return

      for (const file of files) {
        try {
          await uploadAudioClip(file, targetTrackId, dropPosition)
        } catch (err) {
          console.error('Audio upload failed:', err)
        }
      }
    },
    [activeSong, isLiveMode, snapPosition, trackIndexFromY, activeTrackViews],
  )

  // Sync native scroll of tracks container → scrollYRef
  useEffect(() => {
    if (!tracksContainer.current) return
    const container = tracksContainer.current
    function handleContainerScroll() {
      if (isProgrammaticScroll.current) {
        isProgrammaticScroll.current = false
        return
      }
      targetScrollY.current = container.scrollTop
      currentScrollY.current = container.scrollTop
      scrollYRef.current = container.scrollTop
    }
    container.addEventListener('scroll', handleContainerScroll)
    return () => container.removeEventListener('scroll', handleContainerScroll)
  }, [tracksContainer.current])

  const sensors = useSensors(
    useSensor(PointerSensor, {
      activationConstraint: { distance: 5 },
    }),
  )

  const handleDragOver = useCallback(
    (event: DragOverEvent) => {
      const { active, over } = event
      if (!over || active.id === over.id) {
        setDragTrackViews(null)
        return
      }
      const oldIndex = trackViews.findIndex((t) => t.track.id === active.id)
      const newIndex = trackViews.findIndex((t) => t.track.id === over.id)
      if (oldIndex >= 0 && newIndex >= 0) {
        setDragTrackViews(arrayMove(trackViews, oldIndex, newIndex))
      }
    },
    [trackViews],
  )

  const handleDragEnd = useCallback(
    (event: DragEndEvent) => {
      setDragTrackViews(null)
      const { active, over } = event
      if (!over || active.id === over.id) return
      const newIndex = trackViews.findIndex((t) => t.track.id === over.id)
      if (newIndex >= 0) {
        reorderTrack(active.id as string, newIndex)
      }
    },
    [trackViews, reorderTrack],
  )

  const sortableItems = useMemo(
    () => trackViews.map((t) => t.track.id),
    [trackViews],
  )

  const activeSongId = activeSong?.id
  useEffect(() => {
    if (!activeSong) return
    const cursorPosRawPx = (cursorPosRef.current / 60) * activeSong.tempo * 20
    const newScrollX = Math.max(0, cursorPosRawPx - 100)
    targetScrollX.current = newScrollX
    currentScrollX.current = newScrollX
    scrollXRef.current = newScrollX
    zoomRef.current = 1
  }, [activeSongId])

  return (
    project &&
    activeSong && (
      <div className="flex flex-col h-full relative">
        {songLoading && (
          <div className="absolute inset-0 z-50 flex items-center justify-center bg-background/60 backdrop-blur-sm">
            <div className="flex flex-col items-center gap-2 text-sm text-muted-foreground">
              <div className="h-5 w-5 animate-spin rounded-full border-2 border-primary border-t-transparent" />
              Loading...
            </div>
          </div>
        )}
        <Transport />
        <div className="flex flex-row h-full overflow-hidden">
          <div
            ref={tracksContainer}
            className="relative top-5.25 pb-50 pl-1 pr-0.5 flex flex-col w-48 border-t border-r overflow-y-scroll [scrollbar-width:none] [&::-webkit-scrollbar]:hidden"
          >
            <DndContext
              sensors={sensors}
              collisionDetection={closestCenter}
              onDragStart={(event) =>
                setSelectedTrackId(event.active.id as string)
              }
              onDragOver={handleDragOver}
              onDragEnd={handleDragEnd}
              onDragCancel={() => setDragTrackViews(null)}
            >
              <SortableContext
                items={sortableItems}
                strategy={verticalListSortingStrategy}
              >
                {trackViews.map((t) => {
                  const trackComponent = (
                    <SortableTrack
                      trackView={t}
                      availableInputs={availableInputs}
                      trackLevelsRef={trackLevelsRef}
                      selected={selectedTrackId === t.track.id}
                      sortDisabled={isLiveMode}
                      onTrackSelect={setSelectedTrackId}
                      onSetInput={setTrackInput}
                      onSetMonitoring={setTrackMonitoring}
                      onSetMute={setTrackMute}
                      onSetSolo={setTrackSolo}
                      onSetVolume={setTrackVolume}
                      onRename={renameTrack}
                      onSetColor={setTrackColor}
                      onResize={setTrackHeight}
                    />
                  )

                  return isLiveMode ? (
                    trackComponent
                  ) : (
                    <ContextMenu key={t.track.id}>
                      <ContextMenuTrigger asChild>
                        {trackComponent}
                      </ContextMenuTrigger>
                      {!isLiveMode && (
                        <ContextMenuContent>
                          <ContextMenuItem
                            variant="destructive"
                            onClick={() => {
                              setSelectedTrackId(t.track.id)
                              setDeleteDialogOpen(true)
                            }}
                          >
                            Remove track
                          </ContextMenuItem>
                        </ContextMenuContent>
                      )}
                    </ContextMenu>
                  )
                })}
              </SortableContext>
            </DndContext>
            {!isLiveMode && (
              <Button
                variant="ghost"
                size="sm"
                className="w-full text-xs text-muted-foreground justify-center gap-1"
                onClick={() => addTrack()}
              >
                <IconPlus size={14} />
                Add Track
              </Button>
            )}
          </div>

          <div
            className="h-full flex-1"
            style={{ cursor: headerCursor }}
            onDragOver={handleFileDragOver}
            onDragLeave={handleFileDragLeave}
            onDrop={handleFileDrop}
          >
            <Timeline
              draw={draw}
              onWheel={handleWheel}
              onClick={handleClick}
              onMouseMove={handleMouseMove}
              onMouseUp={handleMouseUp}
              onContextMenu={handleContextMenu}
              onKeyDown={handleKeyDown}
            />
          </div>
        </div>

        {endPositionContextMenu && (
          <div
            className="fixed inset-0 z-50"
            onClick={() => setEndPositionContextMenu(null)}
            onContextMenu={(e) => {
              e.preventDefault()
              setEndPositionContextMenu(null)
            }}
          >
            <div
              className="absolute bg-popover border rounded-md shadow-md py-1 min-w-[180px]"
              style={{
                left: endPositionContextMenu.x,
                top: endPositionContextMenu.y,
              }}
            >
              <button
                className="w-full text-left px-3 py-1.5 text-sm hover:bg-accent cursor-default"
                onClick={(e) => {
                  e.stopPropagation()
                  send({
                    action: 'song.setEndPosition',
                    songId: activeSong!.id,
                    endPosition: endPositionContextMenu.position,
                  })
                  setEndPositionContextMenu(null)
                }}
              >
                {activeSong?.endPosition !== undefined
                  ? 'Move end position here'
                  : 'Set end position here'}
              </button>
              {activeSong?.endPosition !== undefined && (
                <button
                  className="w-full text-left px-3 py-1.5 text-sm text-destructive hover:bg-accent cursor-default"
                  onClick={(e) => {
                    e.stopPropagation()
                    send({
                      action: 'song.setEndPosition',
                      songId: activeSong!.id,
                      endPosition: null,
                    })
                    setEndPositionContextMenu(null)
                  }}
                >
                  Remove end position
                </button>
              )}
            </div>
          </div>
        )}

        {markerContextMenu && (
          <div
            className="fixed inset-0 z-50"
            onClick={() => setMarkerContextMenu(null)}
            onContextMenu={(e) => { e.preventDefault(); setMarkerContextMenu(null) }}
          >
            <div
              className="absolute bg-popover border rounded-md shadow-md py-1 min-w-[180px]"
              style={{ left: markerContextMenu.x, top: markerContextMenu.y }}
              onClick={(e) => e.stopPropagation()}
            >
              {markerContextMenu.existingMarker ? (
                <button
                  className="w-full text-left px-3 py-1.5 text-sm text-destructive hover:bg-accent cursor-default"
                  onClick={(e) => {
                    e.stopPropagation()
                    removeMarker(markerContextMenu.existingMarker!.id)
                    setMarkerContextMenu(null)
                  }}
                >
                  Delete marker
                </button>
              ) : (
                <button
                  className="w-full text-left px-3 py-1.5 text-sm hover:bg-accent cursor-default"
                  onClick={(e) => {
                    e.stopPropagation()
                    addMarker('Marker', markerContextMenu.position)
                    setMarkerContextMenu(null)
                  }}
                >
                  Add marker here
                </button>
              )}
              <div className="border-t my-1" />
              <button
                className="w-full text-left px-3 py-1.5 text-sm hover:bg-accent cursor-default"
                onClick={(e) => {
                  e.stopPropagation()
                  send({
                    action: 'song.setEndPosition',
                    songId: activeSong!.id,
                    endPosition: markerContextMenu.position,
                  })
                  setMarkerContextMenu(null)
                }}
              >
                {activeSong?.endPosition !== undefined
                  ? 'Move end position here'
                  : 'Set end position here'}
              </button>
              {activeSong?.endPosition !== undefined && (
                <button
                  className="w-full text-left px-3 py-1.5 text-sm text-destructive hover:bg-accent cursor-default"
                  onClick={(e) => {
                    e.stopPropagation()
                    send({
                      action: 'song.setEndPosition',
                      songId: activeSong!.id,
                      endPosition: null,
                    })
                    setMarkerContextMenu(null)
                  }}
                >
                  Remove end position
                </button>
              )}
            </div>
          </div>
        )}

        {clipContextMenu && (
          <div
            className="fixed inset-0 z-50"
            onClick={() => setClipContextMenu(null)}
            onContextMenu={(e) => {
              e.preventDefault()
              setClipContextMenu(null)
            }}
          >
            <div
              className="absolute bg-popover border rounded-md shadow-md py-1 min-w-[160px]"
              style={{ left: clipContextMenu.x, top: clipContextMenu.y }}
            >
              <button
                className="w-full text-left px-3 py-1.5 text-sm text-destructive hover:bg-accent cursor-default"
                onClick={(e) => {
                  e.stopPropagation()
                  removeSelectedClip()
                  setClipContextMenu(null)
                }}
              >
                Remove clip
              </button>
            </div>
          </div>
        )}

        <AlertDialog open={deleteDialogOpen} onOpenChange={setDeleteDialogOpen}>
          <AlertDialogContent>
            <AlertDialogHeader>
              <AlertDialogTitle>Remove track</AlertDialogTitle>
              <AlertDialogDescription>
                Are you sure you want to remove this track? This action cannot
                be undone.
              </AlertDialogDescription>
            </AlertDialogHeader>
            <AlertDialogFooter>
              <AlertDialogCancel>Cancel</AlertDialogCancel>
              <AlertDialogAction onClick={confirmDelete} variant="destructive">
                Remove
              </AlertDialogAction>
            </AlertDialogFooter>
          </AlertDialogContent>
        </AlertDialog>
      </div>
    )
  )
}

export default Sequencer
