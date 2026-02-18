import { useProject } from '@/shared/contexts/project-provider'
import { Transport } from '../../transport/components/Transport'
import Track from './Track'
import { useCallback, useState } from 'react'
import Timeline from './Timeline'
import { useSequencer } from '../hooks/useSequencer'

function Sequencer() {
  const { project, activeSong } = useProject()
  const [zoom, setZoom] = useState(1)
  const [scrollX, setScrollX] = useState(0)
  const { draw } = useSequencer(zoom, scrollX)

  const handleWheel = useCallback((e: WheelEvent) => {
    e.preventDefault()

    if (e.shiftKey) {
      const scrollDelta = e.deltaY > 0 ? 50 : -50
      setScrollX((prev) => Math.max(0, prev + scrollDelta))
    } else {
      const zoomFactor = e.deltaY > 0 ? 0.9 : 1.1
      const canvasWidth = (e.target as HTMLElement).getBoundingClientRect().width

      setZoom((prevZoom) => {
        const newZoom = Math.max(0.05, Math.min(10, prevZoom * zoomFactor))
        setScrollX((prevScrollX) => {
          const centerContent = prevScrollX + canvasWidth / 2
          const newScrollX = (centerContent / prevZoom) * newZoom - canvasWidth / 2
          return Math.max(0, newScrollX)
        })
        return newZoom
      })
    }
  }, [])

  return (
    project &&
    activeSong && (
      <div className="flex flex-col h-full">
        <Transport />
        <div className="flex flex-row h-full">
          <div className="flex flex-col bg-foreground/5 w-48 border-r border-border">
            {activeSong.tracks.map((t) => {
              return (
                <Track
                  key={t.id}
                  name={t.name}
                  mute={t.mute}
                  solo={t.solo}
                  volume={t.volume}
                />
              )
            })}
          </div>

          <div className="h-full flex-1 border">
            <Timeline draw={draw} onWheel={handleWheel} />
          </div>
        </div>
      </div>
    )
  )
}

export default Sequencer
