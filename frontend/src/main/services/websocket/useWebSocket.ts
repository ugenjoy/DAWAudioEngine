/**
 * WebSocket Hook
 * React hook for managing WebSocket connections
 */

import { useEffect, useRef, useState, useCallback } from 'react'
import type { WebSocketMessage } from '../api/types'

interface UseWebSocketOptions {
  autoConnect?: boolean
  reconnectInterval?: number
  maxReconnectAttempts?: number
}

interface UseWebSocketReturn {
  isConnected: boolean
  error: Error | null
  messages: WebSocketMessage[]
  lastMessage: WebSocketMessage | null
  send: (message: WebSocketMessage) => void
  connect: () => void
  disconnect: () => void
}

const DEFAULT_OPTIONS: UseWebSocketOptions = {
  autoConnect: false,
  reconnectInterval: 3000,
  maxReconnectAttempts: 5
}

/**
 * Custom hook for WebSocket connection management
 *
 * @example
 * const { isConnected, send, messages } = useWebSocket('ws://localhost:8080/ws', {
 *   autoConnect: true
 * })
 */
export function useWebSocket(url: string, options: UseWebSocketOptions = {}): UseWebSocketReturn {
  const opts = { ...DEFAULT_OPTIONS, ...options }

  const [isConnected, setIsConnected] = useState(false)
  const [error, setError] = useState<Error | null>(null)
  const [messages, setMessages] = useState<WebSocketMessage[]>([])
  const [lastMessage, setLastMessage] = useState<WebSocketMessage | null>(null)

  const wsRef = useRef<WebSocket | null>(null)
  const reconnectAttemptsRef = useRef(0)
  const reconnectTimeoutRef = useRef<NodeJS.Timeout | null>(null)

  /**
   * Connect to WebSocket server
   */
  const connect = useCallback(() => {
    // Close existing connection if any
    if (wsRef.current) {
      wsRef.current.close()
    }

    try {
      console.log('[WebSocket] Connecting to', url)
      const ws = new WebSocket(url)

      ws.onopen = () => {
        console.log('[WebSocket] Connected')
        setIsConnected(true)
        setError(null)
        reconnectAttemptsRef.current = 0
      }

      ws.onclose = (event) => {
        console.log('[WebSocket] Disconnected', event.code, event.reason)
        setIsConnected(false)

        // Auto-reconnect logic
        if (opts.maxReconnectAttempts && reconnectAttemptsRef.current < opts.maxReconnectAttempts) {
          reconnectAttemptsRef.current += 1
          console.log(
            `[WebSocket] Reconnecting... (${reconnectAttemptsRef.current}/${opts.maxReconnectAttempts})`
          )
          reconnectTimeoutRef.current = setTimeout(() => {
            connect()
          }, opts.reconnectInterval)
        }
      }

      ws.onerror = (event) => {
        console.error('[WebSocket] Error:', event)
        setError(new Error('WebSocket connection error'))
      }

      ws.onmessage = (event) => {
        try {
          const message: WebSocketMessage = JSON.parse(event.data)
          console.log('[WebSocket] Received:', message)
          setLastMessage(message)
          setMessages((prev) => [...prev, message])
        } catch (err) {
          console.error('[WebSocket] Failed to parse message:', err)
        }
      }

      wsRef.current = ws
    } catch (err) {
      console.error('[WebSocket] Connection failed:', err)
      setError(err instanceof Error ? err : new Error('Connection failed'))
    }
  }, [url, opts.reconnectInterval, opts.maxReconnectAttempts])

  /**
   * Disconnect from WebSocket server
   */
  const disconnect = useCallback(() => {
    console.log('[WebSocket] Disconnecting...')

    // Clear reconnect timeout
    if (reconnectTimeoutRef.current) {
      clearTimeout(reconnectTimeoutRef.current)
      reconnectTimeoutRef.current = null
    }

    // Close WebSocket
    if (wsRef.current) {
      wsRef.current.close()
      wsRef.current = null
    }

    setIsConnected(false)
    reconnectAttemptsRef.current = 0
  }, [])

  /**
   * Send message to WebSocket server
   */
  const send = useCallback((message: WebSocketMessage) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      console.log('[WebSocket] Sending:', message)
      wsRef.current.send(JSON.stringify(message))
    } else {
      console.warn('[WebSocket] Cannot send message - not connected')
      setError(new Error('WebSocket not connected'))
    }
  }, [])

  /**
   * Auto-connect on mount if enabled
   */
  useEffect(() => {
    if (opts.autoConnect) {
      connect()
    }

    // Cleanup on unmount
    return () => {
      disconnect()
    }
  }, [opts.autoConnect, connect, disconnect])

  return {
    isConnected,
    error,
    messages,
    lastMessage,
    send,
    connect,
    disconnect
  }
}
