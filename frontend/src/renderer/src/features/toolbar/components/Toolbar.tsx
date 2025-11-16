import { Button } from '@/shadcn/components/button'
import React from 'react'
import { useWebSocket } from '@/features/webSocket/hooks/useWebSocket'
import { WebSocketStatus } from '@/features/webSocket/components/WebSocketStatus'
import useToolbar from '../hooks/useToolbar'
import { Play, Square } from 'lucide-react'

function Toolbar(): React.JSX.Element {
  const { connectionStatus } = useWebSocket()
  const { testStatus, handlePlayClick, handleStopClick } = useToolbar()

  return (
    <div className="w-full p-2 flex flex-col gap-2 border-border border-b">
      <div className="flex justify-center gap-2">
        <Button onClick={handlePlayClick} variant="ghost" size="icon-sm">
          <Play />
        </Button>
        <Button onClick={handleStopClick} variant="ghost" size="icon-sm">
          <Square />
        </Button>
        <WebSocketStatus className="absolute right-6 top-4.5" status={connectionStatus} />
      </div>
      {testStatus && (
        <div className="text-sm text-center text-muted-foreground px-2">{testStatus}</div>
      )}
    </div>
  )
}

export default Toolbar
