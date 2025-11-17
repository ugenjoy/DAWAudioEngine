import { useState, useEffect } from 'react'
import { useWebSocket } from '@/features/webSocket/hooks/useWebSocket'

interface UseToolbarReturn {
  playing: boolean
  status: string
  handlePlayClick: () => void
  handlePauseClick: () => void
  handleStopClick: () => void
  handleSwitchSong: () => void
  handleGetSongs: () => void
}

function useToolbar(): UseToolbarReturn {
  const { isConnected, send, lastMessage } = useWebSocket()
  const [status, setStatus] = useState<string>('')
  const [playing, setPlaying] = useState<boolean>(false)

  // Listen for echo responses
  useEffect(() => {
    if (lastMessage) {
      setStatus(`✓ Received: ${lastMessage}`)
      const response = JSON.parse(lastMessage)

      if (response.playing !== undefined) {
        setPlaying(response.playing as boolean)
      }

      if (response.songs !== undefined) {
        console.log(response.songs)
      }

      // Clear status after 5 seconds
      setTimeout(() => {
        setStatus('')
      }, 5000)
    }
  }, [lastMessage])

  const handlePlayClick = (): void => {
    sendAction('play')
  }

  const handlePauseClick = (): void => {
    sendAction('pause')
  }

  const handleStopClick = (): void => {
    sendAction('stop')
  }

  const handleSwitchSong = (): void => {
    sendAction('switchSong', { songId: 1 })
  }

  const handleGetSongs = (): void => {
    sendAction('getSongs')
  }

  async function sendAction(action: string, params?: object): Promise<void> {
    if (!isConnected) {
      setStatus('✗ WebSocket not connected')
      setTimeout(() => {
        setStatus('')
      }, 5000)
      return
    }

    try {
      await send({
        type: 'action',
        payload: { action, ...params }
      })
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error'
      setStatus(`✗ Error: ${errorMessage}`)

      setTimeout(() => {
        setStatus('')
      }, 5000)
    }
  }

  return {
    playing,
    status,
    handlePlayClick,
    handlePauseClick,
    handleStopClick,
    handleSwitchSong,
    handleGetSongs
  }
}

export default useToolbar
