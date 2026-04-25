import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useState,
} from 'react'
import { useWebSocket } from './websocket-provider'
import { Marker } from '../models/marker'

type MarkersProviderState = {
  markers: Marker[]
  addMarker: (name: string, position: number) => void
  removeMarker: (markerId: string) => void
  updateMarker: (markerId: string, changes: { name?: string; position?: number }) => void
  fetchMarkers: () => void
}

const initialState: MarkersProviderState = {
  markers: [],
  addMarker: () => null,
  removeMarker: () => null,
  updateMarker: () => null,
  fetchMarkers: () => null,
}

const MarkersContext = createContext<MarkersProviderState>(initialState)

export function MarkersProvider({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  const { ws, isConnected, send } = useWebSocket()
  const [markers, setMarkers] = useState<Marker[]>([])

  const fetchMarkers = useCallback(() => {
    if (ws && isConnected) {
      send({ action: 'marker.list' })
    }
  }, [ws, isConnected, send])

  const addMarker = useCallback(
    (name: string, position: number) => {
      send({ action: 'marker.add', name, position })
    },
    [send],
  )

  const removeMarker = useCallback(
    (markerId: string) => {
      send({ action: 'marker.remove', markerId })
    },
    [send],
  )

  const updateMarker = useCallback(
    (markerId: string, changes: { name?: string; position?: number }) => {
      send({ action: 'marker.update', markerId, ...changes })
    },
    [send],
  )

  useEffect(() => {
    if (ws && isConnected) {
      ws.addEventListener('message', onMessage)
      return () => ws.removeEventListener('message', onMessage)
    }
  }, [ws, isConnected, send])

  function onMessage(ev: MessageEvent<unknown>) {
    if (typeof ev.data !== 'string') return
    const data = JSON.parse(ev.data)
    switch (data.event) {
      case 'marker.listUpdated':
        setMarkers(data.markers ?? [])
        break
      case 'project.currentLoaded':
        setMarkers(data.activeSong?.markers ?? [])
        break
      case 'song.loaded':
        setMarkers(data.song?.markers ?? [])
        break
      case 'song.unloaded':
        setMarkers([])
        break
    }
  }

  return (
    <MarkersContext.Provider
      value={{ markers, addMarker, removeMarker, updateMarker, fetchMarkers }}
    >
      {children}
    </MarkersContext.Provider>
  )
}

export const useMarkers = () => {
  const context = useContext(MarkersContext)
  if (context === undefined)
    throw new Error('useMarkers must be used within a MarkersProvider')
  return context
}
