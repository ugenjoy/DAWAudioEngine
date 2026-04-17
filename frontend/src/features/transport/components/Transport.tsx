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
import { useEffect, useRef, useState } from 'react'

export function Transport() {
  const [bpm, setBpm] = useState<number | string>(120)
  const { send } = useWebSocket()
  const {
    activeSong,
    playheadPosRef,
    playing,
    masterVolume,
    setMasterVolume,
    setTempo,
    setMetronomeMute,
    activeLoop,
    cancelLoop,
    exitLoop,
  } = useProject()
  const { isLiveMode } = useMode()
  const displayRef = useRef<HTMLSpanElement>(null)

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
    setMasterVolume(value[0])
  }

  function commitBpm() {
    const value = Number(bpm) || 120
    const clamped = Math.min(999, Math.max(20, value))
    setBpm(clamped)
    setTempo(clamped)
  }

  // RAF-based playhead display — updates DOM directly, no React re-renders
  useEffect(() => {
    const tempo = activeSong?.tempo || 120
    const updateDisplay = () => {
      const pos = playheadPosRef.current
      const totalBeats = (pos / 60) * tempo
      const bar = Math.floor(totalBeats / 4) + 1
      const beat = Math.floor(totalBeats % 4) + 1
      const sub = Math.floor((totalBeats % 1) * 4) + 1
      if (displayRef.current) {
        displayRef.current.textContent = `${bar}.${beat}.${sub}`
      }
    }

    updateDisplay()
    if (!playing) return

    let rafId: number
    const tick = () => {
      updateDisplay()
      rafId = requestAnimationFrame(tick)
    }
    rafId = requestAnimationFrame(tick)
    return () => cancelAnimationFrame(rafId)
  }, [playing, playheadPosRef, activeSong?.tempo])

  useEffect(() => {
    if (activeSong) {
      setBpm(activeSong.tempo)
    }
  }, [activeSong])

  return (
    activeSong && (
      <div className="flex items-center gap-4 px-3 py-2 bg-card border-b border-border">
        <div className="flex items-center gap-1">
          <Button
            variant="ghost"
            size="icon-sm"
            onClick={() => resetPosition()}
            title="Go to start"
          >
            <IconPlayerSkipBackFilled className="size-4" />
          </Button>

          <Button
            variant="ghost"
            size="icon-sm"
            onClick={() => transport('stop')}
            title="Stop"
          >
            <IconPlayerStopFilled className="size-4" />
          </Button>

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

          <Button
            variant="ghost"
            size="icon-sm"
            disabled
            title="Record (not yet implemented)"
          >
            <IconPlayerRecordFilled className="size-4" />
          </Button>
        </div>

        <Button
          variant="ghost"
          size="icon-sm"
          disabled
          title="Loop (not yet implemented)"
        >
          <IconRepeat className="size-4" />
        </Button>

        <div className="flex items-center gap-2 bg-background/50 px-3 py-1 font-mono text-sm border border-border">
          <span ref={displayRef} className="font-medium tabular-nums">
            1.1.1
          </span>
        </div>

        {/* BPM and time signature are disabled in Live mode */}
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

        {activeLoop && (
          <div className="flex items-center gap-2">
            <span className="text-xs font-medium text-amber-400">LOOP</span>
            <Button
              variant="ghost"
              size="sm"
              onClick={cancelLoop}
              title="Cancel loop (continue past end)"
              className="text-amber-400 hover:text-amber-300"
            >
              Cancel
            </Button>
            <Button
              variant="ghost"
              size="sm"
              onClick={exitLoop}
              title="Exit loop (jump to end)"
              className="text-amber-400 hover:text-amber-300"
            >
              Exit
            </Button>
          </div>
        )}

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
