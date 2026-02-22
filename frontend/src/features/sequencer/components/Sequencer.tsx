import { useProject } from '@/shared/contexts/project-provider'
import { Transport } from '../../transport/components/Transport'
import Track from './Track'
import { useCallback, useEffect, useRef, useState } from 'react'
import Timeline from './Timeline'
import { useSequencer } from '../hooks/useSequencer'
import { useWebSocket } from '@/shared/contexts/websocket-provider'

function Sequencer() {
  const {
    project,
    playing,
    activeSong,
    cursorPos,
    trackViews,
    availableInputs,
    setTrackInput,
    setTrackMonitoring,
    setTrackMute,
    setTrackSolo,
    setTrackVolume,
  } = useProject()
  const { send } = useWebSocket()
  const [zoom, setZoom] = useState(1)
  const [scrollX, setScrollX] = useState(0)
  const [scrollY, setScrollY] = useState(0)
  const { draw } = useSequencer(zoom, scrollX, scrollY)

  const tracksContainer = useRef<HTMLDivElement>(null)
  const isProgrammaticScroll = useRef(false)

  const handleWheel = useCallback(
    (e: WheelEvent) => {
      e.preventDefault()

      if (e.shiftKey) {
        const scrollDelta = e.deltaY > 0 ? 50 : -50
        setScrollX((prev) => Math.max(0, prev + scrollDelta))
      } else if (e.ctrlKey) {
        const zoomFactor = e.deltaY > 0 ? 0.9 : 1.1
        const canvasWidth = (e.target as HTMLElement).getBoundingClientRect()
          .width
        setZoom((prevZoom) => {
          const newZoom = Math.max(0.02, Math.min(100, prevZoom * zoomFactor))
          setScrollX((prevScrollX) => {
            const cursorPosRawPx = (cursorPos / 60) * activeSong!.tempo * 20
            const cursorPosScreenX = cursorPosRawPx * prevZoom - prevScrollX
            const offset = 100
            const anchorScreenX = Math.max(
              offset,
              Math.min(canvasWidth - offset, cursorPosScreenX),
            )

            return Math.max(0, cursorPosRawPx * newZoom - anchorScreenX)
          })
          return newZoom
        })
      } else {
        const scrollDelta = e.deltaY > 0 ? 50 : -50
        const maxScrollY = tracksContainer.current
          ? tracksContainer.current.scrollHeight -
            tracksContainer.current.clientHeight
          : 0
        setScrollY((prev) =>
          Math.max(0, Math.min(maxScrollY, prev + scrollDelta)),
        )
      }
    },
    [cursorPos, activeSong],
  )

  const handleClick = useCallback(
    (e: MouseEvent) => {
      if (!activeSong) return
      e.preventDefault()

      const pixelsPerBeat = 20 * zoom
      let beatsPerLine: number
      if (pixelsPerBeat < 4) {
        beatsPerLine = 16
      } else if (pixelsPerBeat < 16) {
        beatsPerLine = 4
      } else if (pixelsPerBeat > 64) {
        beatsPerLine = 0.25
      } else {
        beatsPerLine = 1
      }

      const rawPos =
        ((e.offsetX + scrollX) * 60) / (activeSong.tempo * 20 * zoom)
      const snapInterval = (beatsPerLine * 60) / activeSong.tempo
      const cursorPos = Math.round(rawPos / snapInterval) * snapInterval

      send({
        action: 'transport.setCursorPosition',
        position: cursorPos,
      })
    },
    [zoom, scrollX, activeSong],
  )

  const handleKeyDown = useCallback(
    (e: KeyboardEvent) => {
      if (!activeSong) return

      switch (e.key) {
        case ' ': {
          e.preventDefault()
          send({
            action: `transport.${playing ? 'stop' : 'play'}`,
          })
          break
        }
      }
    },
    [activeSong, playing],
  )

  useEffect(() => {
    if (!tracksContainer.current) return
    isProgrammaticScroll.current = true
    tracksContainer.current.scrollTo({ top: scrollY, behavior: 'instant' })
  }, [scrollY])

  useEffect(() => {
    if (!tracksContainer.current) return
    const container = tracksContainer.current
    function handleContainerScroll() {
      if (isProgrammaticScroll.current) {
        isProgrammaticScroll.current = false
        return
      }
      setScrollY(container.scrollTop)
    }
    container.addEventListener('scroll', handleContainerScroll)
    return () => container.removeEventListener('scroll', handleContainerScroll)
  }, [tracksContainer.current])

  useEffect(() => {
    if (!activeSong) return
    setScrollX(() => {
      const cursorPosRawPx = (cursorPos / 60) * activeSong.tempo * 20
      return Math.max(0, cursorPosRawPx - 100)
    })
    setZoom(1)
  }, [activeSong])

  return (
    project &&
    activeSong && (
      <div className="flex flex-col h-full">
        <Transport />
        <div className="flex flex-row h-full overflow-hidden">
          <div
            ref={tracksContainer}
            className="relative top-5.25 pb-50 flex flex-col w-48 border-r overflow-y-scroll [scrollbar-width:none] [&::-webkit-scrollbar]:hidden"
          >
            {trackViews.map((t) => {
              return (
                <Track
                  key={t.track.id}
                  id={t.track.id}
                  name={t.track.name}
                  mute={t.track.mute}
                  solo={t.track.solo}
                  volume={t.track.volume}
                  color={t.strokeColor}
                  height={t.height}
                  inputChannel={t.track.inputChannel ?? -1}
                  inputStereo={t.track.inputStereo ?? false}
                  monitoring={t.track.monitoring ?? false}
                  availableInputs={availableInputs}
                  onSetInput={setTrackInput}
                  onSetMonitoring={setTrackMonitoring}
                  onSetMute={setTrackMute}
                  onSetSolo={setTrackSolo}
                  onSetVolume={setTrackVolume}
                />
              )
            })}
          </div>

          <div className="h-full flex-1 border">
            <Timeline
              draw={draw}
              onWheel={handleWheel}
              onClick={handleClick}
              onKeyDown={handleKeyDown}
            />
          </div>
        </div>
      </div>
    )
  )
}

export default Sequencer
