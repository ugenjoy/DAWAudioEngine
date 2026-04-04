import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useState,
} from 'react'
import { useWebSocket } from './websocket-provider'
import { Setlist, SetlistEntry } from '../models/setlist'
import { EventRule } from '../models/event-rule'
import { Song } from '../models/song'

type SetlistProviderState = {
  setlists: Setlist[]
  activeSetlist: Setlist | null
  currentIndex: number
  currentSong: Song | null
  advance: () => void
  previous: () => void
  goTo: (index: number) => void
  loadSetlist: (setlistId: string) => void
  loadSingle: (songId: string) => void
  unload: () => void
  createSetlist: (name: string, entries: SetlistEntry[], events: EventRule[]) => void
  updateSetlist: (id: string, name?: string, entries?: SetlistEntry[], events?: EventRule[]) => void
  deleteSetlist: (id: string) => void
}

const initialState: SetlistProviderState = {
  setlists: [],
  activeSetlist: null,
  currentIndex: 0,
  currentSong: null,
  advance: () => null,
  previous: () => null,
  goTo: () => null,
  loadSetlist: () => null,
  loadSingle: () => null,
  unload: () => null,
  createSetlist: () => null,
  updateSetlist: () => null,
  deleteSetlist: () => null,
}

const SetlistContext = createContext<SetlistProviderState>(initialState)

export function SetlistProvider({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  const { ws, isConnected, send } = useWebSocket()
  const [setlists, setSetlists] = useState<Setlist[]>([])
  const [activeSetlist, setActiveSetlist] = useState<Setlist | null>(null)
  const [currentIndex, setCurrentIndex] = useState(0)
  const [currentSong, setCurrentSong] = useState<Song | null>(null)

  useEffect(() => {
    if (!ws || !isConnected) return
    ws.addEventListener('message', onMessage)
    send({ action: 'setlist.list' })
    return () => ws.removeEventListener('message', onMessage)
  }, [ws, isConnected])

  function onMessage(ev: MessageEvent<unknown>) {
    if (typeof ev.data !== 'string') return
    const data = JSON.parse(ev.data)

    switch (data.event) {
      case 'setlist.listUpdated':
        setSetlists(data.setlists ?? [])
        break
      case 'setlist.loaded':
        setActiveSetlist(data.setlist)
        setCurrentIndex(data.currentIndex)
        setCurrentSong(data.song)
        break
      case 'setlist.unloaded':
        setActiveSetlist(null)
        setCurrentSong(null)
        break
      case 'setlist.songChanged':
        setCurrentIndex(data.currentIndex)
        setCurrentSong(data.song)
        break
      case 'project.currentLoaded':
        if (data.setlist) {
          setActiveSetlist(data.setlist)
          setCurrentIndex(data.currentIndex ?? 0)
          setCurrentSong(data.activeSong ?? null)
        }
        break
      case 'project.loaded':
        if (data.setlists) setSetlists(data.setlists)
        break
    }
  }

  const advance = useCallback(() => send({ action: 'setlist.advance' }), [send])
  const previous = useCallback(() => send({ action: 'setlist.previous' }), [send])
  const goTo = useCallback((index: number) => send({ action: 'setlist.goTo', index }), [send])
  const loadSetlist = useCallback((setlistId: string) => send({ action: 'setlist.load', setlistId }), [send])
  const loadSingle = useCallback((songId: string) => send({ action: 'setlist.loadSingle', songId }), [send])
  const unload = useCallback(() => send({ action: 'setlist.unload' }), [send])

  const createSetlist = useCallback(
    (name: string, entries: SetlistEntry[], events: EventRule[]) =>
      send({ action: 'setlist.create', name, entries, events }),
    [send],
  )

  const updateSetlist = useCallback(
    (id: string, name?: string, entries?: SetlistEntry[], events?: EventRule[]) =>
      send({ action: 'setlist.update', setlistId: id, name, entries, events }),
    [send],
  )

  const deleteSetlist = useCallback(
    (id: string) => send({ action: 'setlist.delete', setlistId: id }),
    [send],
  )

  return (
    <SetlistContext.Provider
      value={{
        setlists,
        activeSetlist,
        currentIndex,
        currentSong,
        advance,
        previous,
        goTo,
        loadSetlist,
        loadSingle,
        unload,
        createSetlist,
        updateSetlist,
        deleteSetlist,
      }}
    >
      {children}
    </SetlistContext.Provider>
  )
}

export const useSetlist = () => {
  const context = useContext(SetlistContext)
  if (context === undefined)
    throw new Error('useSetlist must be used within a SetlistProvider')
  return context
}
