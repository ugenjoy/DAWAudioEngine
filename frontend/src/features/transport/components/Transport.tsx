import { useProject } from '@/shared/contexts/project-provider'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { Button } from '@/shared/shadcn/components/button'
import { Input } from '@/shared/shadcn/components/input'
import {
  IconPlayerPlayFilled,
  IconPlayerStopFilled,
  IconPlayerRecordFilled,
  IconPlayerSkipBackFilled,
  IconPlayerSkipForwardFilled,
  IconRepeat,
  IconMetronome,
  IconPlayerPauseFilled,
} from '@tabler/icons-react'
import { useEffect, useState } from 'react'

export function Transport() {
  const [isPlaying, setIsPlaying] = useState(false)
  const [isRecording, setIsRecording] = useState(false)
  const [isLooping, setIsLooping] = useState(true)
  const [isMetronomeOn, setIsMetronomeOn] = useState(false)
  const [bpm, setBpm] = useState(120)
  const [position, setPosition] = useState('1.1.1')

  const { send, ws } = useWebSocket()
  const { activeSong } = useProject()

  function onMessage(ev: MessageEvent<string>) {
    const data = JSON.parse(ev.data)
    switch (data.event) {
      case 'transport.play': {
        setIsPlaying(true)
        break
      }
      case 'transport.pause': {
        setIsPlaying(false)
        break
      }
      case 'transport.stop': {
        setIsPlaying(false)
        break
      }
    }
  }

  function transport(action: 'play' | 'pause' | 'stop') {
    send({
      action: `transport.${action}`,
    })
  }

  useEffect(() => {
    if (activeSong) {
      setBpm(activeSong.tempo)
    }
  }, [activeSong])

  useEffect(() => {
    if (ws) {
      ws.addEventListener('message', onMessage)
      return () => ws.removeEventListener('message', onMessage)
    }
  }, [ws])

  return (
    <div className="flex items-center gap-4 px-3 py-2 bg-card border-b border-border">
      {/* Transport controls */}
      <div className="flex items-center gap-1">
        <Button
          variant="ghost"
          size="icon-sm"
          onClick={() => setPosition('1.1.1')}
          title="Go to start"
        >
          <IconPlayerSkipBackFilled className="size-4" />
        </Button>

        {/* Play / Pause button */}
        <Button
          variant={isPlaying ? 'default' : 'ghost'}
          size="icon-sm"
          onClick={() => transport(isPlaying ? 'pause' : 'play')}
          title={isPlaying ? 'Stop' : 'Play'}
        >
          {isPlaying ? (
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

        {/* Stop button */}
        <Button
          variant="ghost"
          size="icon-sm"
          onClick={() => transport('stop')}
          title="Stop"
        >
          <IconPlayerStopFilled className="size-4" />
        </Button>

        <Button variant="ghost" size="icon-sm" title="Go to end">
          <IconPlayerSkipForwardFilled className="size-4" />
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
        <span className="text-muted-foreground text-xs">POS</span>
        <span className="font-medium tabular-nums">{position}</span>
      </div>

      {/* BPM control */}
      <div className="flex items-center gap-2">
        <span className="text-muted-foreground text-xs">BPM</span>
        <Input
          type="number"
          value={bpm}
          onChange={(e) => setBpm(Number(e.target.value))}
          className="w-18 h-7 font-mono tabular-nums"
          min={20}
          max={999}
        />
      </div>

      {/* Time signature */}
      <div className="flex items-center gap-2 text-sm">
        <span className="text-muted-foreground text-xs">SIG</span>
        <span className="font-mono">4/4</span>
      </div>

      {/* Metronome */}
      <Button
        variant={isMetronomeOn ? 'secondary' : 'ghost'}
        size="icon-sm"
        onClick={() => setIsMetronomeOn(!isMetronomeOn)}
        title="Metronome"
        className={isMetronomeOn ? 'text-primary' : ''}
      >
        <IconMetronome className="size-4" />
      </Button>

      {/* Spacer */}
      <div className="flex-1" />

      {/* CPU & Audio info */}
      <div className="flex items-center gap-4 text-xs text-muted-foreground">
        <span>CPU: 12%</span>
        <span>44.1kHz / 24bit</span>
      </div>
    </div>
  )
}
