import { useState, useEffect } from 'react'
import { useWebSocket } from '@/features/webSocket/hooks/useWebSocket'

interface UseToolbarReturn {
  testStatus: string
  handlePlayClick: () => void
  handleStopClick: () => void
}

function useToolbar(): UseToolbarReturn {
  const { isConnected, send, lastMessage } = useWebSocket()
  const [testStatus, setTestStatus] = useState<string>('')
  const [isTesting, setIsTesting] = useState(false)

  // Listen for echo responses
  useEffect(() => {
    if (lastMessage && isTesting) {
      setTestStatus(`✓ Received: ${lastMessage}`)
      setIsTesting(false)

      // Clear status after 5 seconds
      setTimeout(() => {
        setTestStatus('')
      }, 5000)
    }
  }, [lastMessage, isTesting])

  const handlePlayClick = (): void => {
    sendAction('play')
  }

  const handleStopClick = (): void => {
    sendAction('stop')
  }

  async function sendAction(action: string): Promise<void> {
    if (!isConnected) {
      setTestStatus('✗ WebSocket not connected')
      setTimeout(() => {
        setTestStatus('')
      }, 5000)
      return
    }

    try {
      // Send play message
      await send({
        type: 'action',
        payload: { action }
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

  return { testStatus, handlePlayClick, handleStopClick }
}

export default useToolbar
