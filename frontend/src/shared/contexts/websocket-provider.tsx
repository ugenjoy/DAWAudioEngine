import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react'
import { WebSocketMessage } from '../services/websocket/command'
import { useNavigate } from 'react-router'

const AUTO_CONNECT = import.meta.env.VITE_WS_AUTO_CONNECT === 'true'

type WebSocketProviderProps = {
  children: React.ReactNode
}

type WebSocketProviderState = {
  ws: WebSocket | null
  isConnected: boolean
  isLoading: boolean
  autoConnect: boolean
  connect: (url: string) => void
  disconnect: () => void
  send: (command: WebSocketMessage) => void
}

const initialState: WebSocketProviderState = {
  ws: null,
  isConnected: false,
  isLoading: false,
  autoConnect: AUTO_CONNECT,
  connect: () => undefined,
  disconnect: () => undefined,
  send: () => undefined,
}

const WebSocketProviderContext =
  createContext<WebSocketProviderState>(initialState)

export function WebSocketProvider({
  children,
  ...props
}: Readonly<WebSocketProviderProps>) {
  const [ws, setWs] = useState<WebSocket | null>(null)
  const [isConnected, setIsConnected] = useState<boolean>(false)
  const [isLoading, setIsLoading] = useState<boolean>(false)
  const navigate = useNavigate()
  const reconnectTimer = useRef<ReturnType<typeof setTimeout> | null>(null)

  function createSocket(url: string) {
    const newWs = new WebSocket(url)
    newWs.onopen = () => {
      setIsLoading(false)
      setIsConnected(true)
      navigate('/')
    }
    newWs.onclose = () => {
      setWs(null)
      setIsConnected(false)
      if (AUTO_CONNECT) {
        reconnectTimer.current = setTimeout(() => {
          createSocket(url)
        }, 2000)
      }
    }
    newWs.onerror = () => {
      setWs(null)
      setIsLoading(false)
    }
    setWs(newWs)
  }

  useEffect(() => {
    if (AUTO_CONNECT) {
      const protocol =
        globalThis.location.protocol === 'https:' ? 'wss:' : 'ws:'
      const url = `${protocol}//${globalThis.location.host}/ws`
      setIsLoading(true)
      createSocket(url)
    }
    return () => {
      if (reconnectTimer.current) clearTimeout(reconnectTimer.current)
    }
  }, [])

  const disconnect = useCallback(() => {
    if (reconnectTimer.current) clearTimeout(reconnectTimer.current)
    if (ws) {
      ws.close()
      setWs(null)
    }
  }, [ws])

  const connect = useCallback((url: string) => {
    setIsLoading(true)
    createSocket(url)
  }, [])

  const send = useCallback(
    (command: WebSocketMessage) => {
      if (ws?.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(command))
      }
    },
    [ws],
  )

  const value: WebSocketProviderState = useMemo(() => {
    return {
      ws,
      isConnected,
      isLoading,
      autoConnect: AUTO_CONNECT,
      connect,
      disconnect,
      send,
    }
  }, [ws, isConnected, isLoading, connect, disconnect, send])

  return (
    <WebSocketProviderContext.Provider {...props} value={value}>
      {children}
    </WebSocketProviderContext.Provider>
  )
}

export const useWebSocket = () => {
  const context = useContext(WebSocketProviderContext)
  if (context === undefined)
    throw new Error('useWebSocket must be used within a WebSocketProvider')
  return context
}
