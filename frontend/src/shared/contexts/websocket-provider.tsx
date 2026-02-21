import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useState,
} from 'react'
import { WebSocketMessage } from '../services/websocket/command'

type WebSocketProviderProps = {
  children: React.ReactNode
}

type WebSocketProviderState = {
  ws: WebSocket | undefined | null
  isConnected: boolean
  isLoading: boolean
  connect: (url: string) => void
  send: (command: WebSocketMessage) => void
}

const initialState: WebSocketProviderState = {
  ws: undefined,
  isConnected: false,
  isLoading: false,
  connect: () => undefined,
  send: () => undefined,
}

const WebSocketProviderContext =
  createContext<WebSocketProviderState>(initialState)

export function WebSocketProvider({
  children,
  ...props
}: Readonly<WebSocketProviderProps>) {
  const [ws, setWs] = useState<WebSocket | null | undefined>()
  const [isConnected, setIsConnected] = useState<boolean>(false)
  const [isLoading, setIsLoading] = useState<boolean>(false)

  useEffect(() => {
    if (ws) {
      ws.onopen = () => {
        setIsLoading(false)
        setIsConnected(true)
        console.log('WebSocket connected')
      }
      ws.onclose = () => {
        setIsConnected(false)
        setWs(null)
        console.log('WebSocket disconnected')
      }
      ws.onerror = () => {
        if (isLoading) setIsLoading(false)
        if (ws) setWs(null)
      }
    }
  }, [ws])

  const connect = useCallback(
    (url: string) => {
      if (!ws) {
        setIsLoading(true)
        setWs(new WebSocket(url))
      }
    },
    [ws],
  )

  const send = useCallback(
    (command: WebSocketMessage) => {
      if (ws?.readyState === WebSocket.OPEN) {
        const strCmd = JSON.stringify(command)
        ws.send(strCmd)
      }
    },
    [ws],
  )

  const value: WebSocketProviderState = useMemo(() => {
    return {
      ws,
      isConnected,
      isLoading,
      connect,
      send,
    }
  }, [ws, isConnected, connect, send])

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
