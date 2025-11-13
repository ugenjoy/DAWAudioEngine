/**
 * WebSocket Hook for Renderer
 * Uses IPC to communicate with WebSocket service in main process
 */

import { useState, useEffect } from 'react'

export type ConnectionStatus = 'disconnected' | 'connecting' | 'connected' | 'error'

interface UseWebSocketReturn {
  connectionStatus: ConnectionStatus
  isConnected: boolean
  lastMessage: string | null
  error: string | null
  send: (message: string | object) => Promise<void>
}

/**
 * Hook to access WebSocket connection via IPC
 */
export function useWebSocket(): UseWebSocketReturn {
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('disconnected')
  const [lastMessage, setLastMessage] = useState<string | null>(null)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    // Get initial status
    globalThis.api.websocket.getStatus().then((status) => {
      setConnectionStatus(status as ConnectionStatus)
    })

    // Listen for status changes
    const unsubscribeStatus = globalThis.api.websocket.onStatus((status) => {
      setConnectionStatus(status as ConnectionStatus)
      if (status === 'connected') {
        setError(null)
      }
    })

    // Listen for messages
    const unsubscribeMessage = globalThis.api.websocket.onMessage((data) => {
      setLastMessage(data)
    })

    // Listen for errors
    const unsubscribeError = globalThis.api.websocket.onError((message) => {
      setError(message)
    })

    // Cleanup listeners on unmount
    return () => {
      unsubscribeStatus()
      unsubscribeMessage()
      unsubscribeError()
    }
  }, [])

  const send = async (message: string | object): Promise<void> => {
    try {
      await globalThis.api.websocket.send(message)
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Failed to send message'
      setError(errorMessage)
    }
  }

  return {
    connectionStatus,
    isConnected: connectionStatus === 'connected',
    lastMessage,
    error,
    send
  }
}
