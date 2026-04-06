import { useProject } from '@/shared/contexts/project-provider'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { Button } from '@/shared/shadcn/components/button'

import {
  IconPlayerPlayFilled,
  IconPlayerStopFilled,
  IconPlayerSkipBackFilled,
  IconPlayerPauseFilled,
} from '@tabler/icons-react'

export function LiveTransport() {
  const { send } = useWebSocket()
  const { activeSong, playing, activeLoop, cancelLoop, exitLoop } = useProject()

  function transport(action: 'play' | 'pause' | 'stop') {
    send({ action: `transport.${action}` })
  }

  function resetPosition() {
    send({ action: `transport.setCursorPosition`, position: 0 })
  }

  return (
    activeSong && (
      <div className="flex items-center gap-4 px-3 py-2 bg-card border-b border-border">
        <div className="flex-1" />
        <div className="flex items-center gap-1">
          <Button
            variant="ghost"
            size="icon-lg"
            onClick={resetPosition}
            title="Go to start"
          >
            <IconPlayerSkipBackFilled className="size-5" />
          </Button>

          <Button
            variant="ghost"
            size="icon-lg"
            onClick={() => transport('stop')}
            title="Stop"
          >
            <IconPlayerStopFilled className="size-5" />
          </Button>

          <Button
            variant={playing ? 'default' : 'ghost'}
            size="icon-lg"
            onClick={() => transport(playing ? 'pause' : 'play')}
            title={playing ? 'Pause' : 'Play'}
          >
            {playing ? (
              <IconPlayerPauseFilled className="size-5" />
            ) : (
              <IconPlayerPlayFilled className="size-5" />
            )}
          </Button>
        </div>

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

        <div className="flex-1" />
      </div>
    )
  )
}
