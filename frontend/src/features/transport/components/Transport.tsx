import { useProject } from '@/shared/contexts/project-provider'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { useMode } from '@/shared/contexts/mode-provider'
import { Button } from '@/shared/shadcn/components/button'
import { Input } from '@/shared/shadcn/components/input'
import { Slider } from '@/shared/shadcn/components/slider'
import {
  IconPlayerPlayFilled,
  IconPlayerStopFilled,
  IconPlayerRecordFilled,
  IconPlayerSkipBackFilled,
  IconRepeat,
  IconMetronome,
  IconPlayerPauseFilled,
  IconVolume,
} from '@tabler/icons-react'
import { useEffect, useState } from 'react'

export function Transport() {
  const [isRecording, setIsRecording] = useState(false)
  const [isLooping, setIsLooping] = useState(false)
  const [bpm, setBpm] = useState<number | string>(120)
  const [masterVolume, setMasterVolume] = useState(1.0)
  const { send } = useWebSocket()
  const {
    activeSong,
    playheadPos: transportPos,
    playing,
    setTempo,
    setMetronomeMute,
  } = useProject()
  const { isLiveMode } = useMode()

  function transport(action: 'play' | 'pause' | 'stop') {
    send({
      action: `transport.${action}`,
    })
  }

  function resetPosition() {
    send({
      action: `transport.setCursorPosition`,
      position: 0,
    })
  }

  function handleMasterVolume(value: number[]) {
    const vol = value[0]
    setMasterVolume(vol)
    send({ action: 'transport.setMasterVolume', volume: vol })
  }

  function commitBpm() {
    const value = Number(bpm) || 120
    const clamped = Math.min(999, Math.max(20, value))
    setBpm(clamped)
    setTempo(clamped)
  }

  function getBarPosition() {
    const b = Number(bpm) || 120
    return Math.floor(((transportPos / 60) * b) / 4) + 1
  }

  function getBeatPosition() {
    const b = Number(bpm) || 120
    return Math.floor(((transportPos / 60) * b) % 4) + 1
  }

  function getSubPosition() {
    const b = Number(bpm) || 120
    return Math.floor((((transportPos / 60) * b) % 1) * 4) + 1
  }

  useEffect(() => {
    if (activeSong) {
      setBpm(activeSong.tempo)
    }
  }, [activeSong])

  return (
    activeSong && (
      <div className="flex items-center gap-4 px-3 py-2 bg-card border-b border-border">
        {/* Transport controls */}
        <div className="flex items-center gap-1">
          <Button
            variant="ghost"
            size="icon-sm"
            onClick={() => resetPosition()}
            title="Go to start"
          >
            <IconPlayerSkipBackFilled className="size-4" />
          </Button>

          {/* Stop button */}
          <Button
            variant="ghost"
            size="icon-sm"
            onClick={() => transport('stop')}
            title="Stop"
          >
            <IconPlayerStopFilled className="size-4" />
          </Button>

          {/* Play / Pause button */}
          <Button
            variant={playing ? 'default' : 'ghost'}
            size="icon-sm"
            onClick={() => transport(playing ? 'pause' : 'play')}
            title={playing ? 'Stop' : 'Play'}
          >
            {playing ? (
              <IconPlayerPauseFilled className="size-4" />
            ) : (
              <IconPlayerPlayFilled className="size-4" />
            )}
          </Button>

          {/* Record button */}
          <Button
            variant={isRecording ? 'destructive' : 'ghost'}
            size="icon-sm"
            onClick={() => setIsRecording(!isRecording)}
            title="Record"
            className={isRecording ? 'text-red-500' : ''}
          >
            <IconPlayerRecordFilled className="size-4" />
          </Button>
        </div>

        {/* Loop toggle */}
        <Button
          variant={isLooping ? 'secondary' : 'ghost'}
          size="icon-sm"
          onClick={() => setIsLooping(!isLooping)}
          title="Loop"
          className={isLooping ? 'text-primary' : ''}
        >
          <IconRepeat className="size-4" />
        </Button>

        {/* Position display */}
        <div className="flex items-center gap-2 bg-background/50 px-3 py-1 font-mono text-sm border border-border">
          <span className="font-medium tabular-nums">
            {getBarPosition()}.{getBeatPosition()}.{getSubPosition()}
          </span>
        </div>

        {/* BPM control — disabled in Live mode */}
        <div className="flex items-center gap-2">
          <span className="text-muted-foreground text-xs">BPM</span>
          <Input
            type="number"
            value={bpm}
            onChange={(e) => setBpm(e.target.value === '' ? '' : Number(e.target.value))}
            onBlur={() => commitBpm()}
            onKeyDown={(e) => {
              if (e.key === 'Enter') {
                commitBpm()
                e.currentTarget.blur()
              }
            }}
            className="w-18 h-7 font-mono tabular-nums"
            min={20}
            max={999}
            disabled={isLiveMode}
          />
        </div>

        {/* Time signature — disabled in Live mode */}
        <div className="flex items-center gap-2 text-sm">
          <span className="text-muted-foreground text-xs">SIG</span>
          <span
            className={
              isLiveMode ? 'font-mono text-muted-foreground' : 'font-mono'
            }
          >
            4/4
          </span>
        </div>

        {/* Metronome toggle — disabled in Live mode */}
        <Button
          variant={!activeSong.metronomeMute ? 'secondary' : 'ghost'}
          size="icon-sm"
          onClick={() => setMetronomeMute(!activeSong.metronomeMute)}
          title="Metronome"
          className={!activeSong.metronomeMute ? 'text-primary' : ''}
          disabled={isLiveMode}
        >
          <IconMetronome className="size-4" />
        </Button>

        <div className="flex-1" />

        {/* Master volume — always available */}
        <div className="flex items-center gap-2">
          <IconVolume size={14} className="text-muted-foreground shrink-0" />
          <Slider
            value={[masterVolume]}
            onValueChange={handleMasterVolume}
            min={0}
            max={1}
            step={0.01}
            className="w-24"
          />
        </div>
      </div>
    )
  )
}
