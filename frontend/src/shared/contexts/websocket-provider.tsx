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
  ws: WebSocket | undefined
  isConnected: boolean
  connect: (url: string) => void
  send: (command: WebSocketMessage) => void
}

const initialState: WebSocketProviderState = {
  ws: undefined,
  isConnected: false,
  connect: () => undefined,
  send: () => undefined,
}

const WebSocketProviderContext =
  createContext<WebSocketProviderState>(initialState)

export function WebSocketProvider({
  children,
  ...props
}: Readonly<WebSocketProviderProps>) {
  const [ws, setWs] = useState<WebSocket>()
  const [isConnected, setIsConnected] = useState<boolean>(false)

  useEffect(() => {
    if (ws) {
      ws.onopen = () => {
        setIsConnected(true)
        console.log('WebSocket connected')
      }
      ws.onclose = () => console.log('WebSocket disconnected')
      ws.onmessage = (ev) =>
        console.log('WebSocket message : ', JSON.parse(ev.data))
    }
  }, [ws])

  const connect = useCallback(
    (url: string) => {
      if (!ws) {
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
