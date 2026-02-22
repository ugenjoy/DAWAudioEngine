import { createContext, useContext, useEffect, useState } from 'react'
import { useWebSocket } from './websocket-provider'
import { useProject } from './project-provider'

type AppMode = 'live' | 'edit'

type ModeProviderState = {
  mode: AppMode
  isLiveMode: boolean
  isEditMode: boolean
  setEditMode: () => void
  setLiveMode: () => void
}

const initialState: ModeProviderState = {
  mode: 'live',
  isLiveMode: true,
  isEditMode: false,
  setEditMode: () => null,
  setLiveMode: () => null,
}

const ModeProviderContext = createContext<ModeProviderState>(initialState)

export function ModeProvider({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  const { ws, isConnected, send } = useWebSocket()
  const { playing } = useProject()
  const [mode, setMode] = useState<AppMode>('live')

  useEffect(() => {
    if (!ws || !isConnected) return

    ws.addEventListener('message', onMessage)
    send({ action: 'mode.getMode' })

    return () => ws.removeEventListener('message', onMessage)
  }, [ws, isConnected])

  function onMessage(ev: MessageEvent<unknown>) {
    if (typeof ev.data !== 'string') return
    const data = JSON.parse(ev.data)

    if (data.event === 'mode.changed' || data.event === 'mode.current') {
      if (data.mode === 'live' || data.mode === 'edit') {
        setMode(data.mode)
      }
    }
  }

  function setEditMode() {
    if (playing) return
    send({ action: 'mode.setEdit' })
  }

  function setLiveMode() {
    send({ action: 'mode.setLive' })
  }

  const value: ModeProviderState = {
    mode,
    isLiveMode: mode === 'live',
    isEditMode: mode === 'edit',
    setEditMode,
    setLiveMode,
  }

  return (
    <ModeProviderContext.Provider value={value}>
      {children}
    </ModeProviderContext.Provider>
  )
}

export const useMode = () => {
  const context = useContext(ModeProviderContext)
  if (context === undefined)
    throw new Error('useMode must be used within a ModeProvider')
  return context
}
