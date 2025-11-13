import { Button } from '@/shadcn/components/button'
import React, { useState, useEffect } from 'react'
import { useWebSocket } from '@/hooks/useWebSocket'
import { WebSocketStatus } from '@/components/WebSocketStatus'

function Toolbar(): React.JSX.Element {
  const { isConnected, send, lastMessage } = useWebSocket()
  const [testStatus, setTestStatus] = useState<string>('')
  const [isTesting, setIsTesting] = useState(false)
  const { connectionStatus } = useWebSocket()

  // Listen for echo responses
  useEffect(() => {
    if (lastMessage && isTesting) {
      // eslint-disable-next-line react-hooks/set-state-in-effect
      setTestStatus(`✓ Received: ${lastMessage}`)
      setIsTesting(false)

      // Clear status after 5 seconds
      setTimeout(() => {
        setTestStatus('')
      }, 5000)
    }
  }, [lastMessage, isTesting])

  const handleTestClick = async (): Promise<void> => {
    if (!isConnected) {
      setTestStatus('✗ WebSocket not connected')
      setTimeout(() => {
        setTestStatus('')
      }, 5000)
      return
    }

    setIsTesting(true)
    setTestStatus('Sending test message...')

    try {
      // Send test message
      await send({
        type: 'test',
        payload: { message: 'Hello from Electron!', timestamp: Date.now() }
      })
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error'
      setTestStatus(`✗ Error: ${errorMessage}`)
      setIsTesting(false)

      setTimeout(() => {
        setTestStatus('')
      }, 5000)
    }
  }

  return (
    <div className="w-full p-2 flex flex-col gap-2 border-border border-b">
      <div className="flex justify-center gap-2">
        <Button>Play</Button>
        <Button>Stop</Button>
        <Button onClick={handleTestClick} variant="outline" disabled={isTesting}>
          {isTesting ? 'Testing...' : 'Test'}
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
