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
  const { activeSong, playing } = useProject()

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

  return (
    activeSong && (
      <div className="flex items-center gap-4 px-3 py-2 bg-card border-b border-border">
        <div className="flex-1" />
        <div className="flex items-center gap-1">
          <Button
            variant="ghost"
            size="icon-lg"
            onClick={() => resetPosition()}
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
            title={playing ? 'Stop' : 'Play'}
          >
            {playing ? (
              <IconPlayerPauseFilled className="size-5" />
            ) : (
              <IconPlayerPlayFilled className="size-5" />
            )}
          </Button>
        </div>
        <div className="flex-1" />
      </div>
    )
  )
}
