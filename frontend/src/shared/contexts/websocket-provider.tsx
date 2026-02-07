import { createContext, useContext, useEffect, useMemo, useState } from 'react'
import { WebSocketMessage } from '../services/api/types'

type WebSocketProviderProps = {
  children: React.ReactNode
}

type WebSocketProviderState = {
  ws: WebSocket | undefined
  connect: (url: string) => void
  send: (command: WebSocketMessage) => void
}

const initialState: WebSocketProviderState = {
  ws: undefined,
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

  useEffect(() => {
    if (ws) {
      ws.onopen = () => console.log('WebSocket connected')
      ws.onclose = () => console.log('WebSocket disconnected')
      ws.onmessage = (ev) => console.log('WebSocket message : ', ev.data)
    }
  }, [ws])

  function connect(url: string) {
    if (!ws) {
      setWs(new WebSocket(url))
    }
  }

  function send(command: WebSocketMessage) {
    if (ws) {
      const strCmd = JSON.stringify(command)
      ws.send(strCmd)
    }
  }

  const value: WebSocketProviderState = useMemo(() => {
    return {
      ws,
      connect,
      send,
    }
  }, [ws, connect, send])

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
