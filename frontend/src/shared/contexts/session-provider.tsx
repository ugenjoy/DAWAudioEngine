import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useRef,
  useState,
} from 'react'
import { useNavigate } from 'react-router'
import { useWebSocket } from './websocket-provider'
import { Project } from '../models/project'
import { Song } from '../models/song'

export type SessionActivity = 'editing' | 'performing' | 'none'

export interface SessionEditing {
  songId: string
  song: Song | null
}

export interface SessionPerforming {
  setlistId: string
  currentIndex: number
  song: Song | null
}

export interface SessionPlayback {
  isPlaying: boolean
  playheadPosition: number
  cursorPosition: number
  masterVolume: number
}

export interface SessionState {
  hasProject: boolean
  project: Project | null
  activity: SessionActivity
  editing: SessionEditing | null
  performing: SessionPerforming | null
  playback: SessionPlayback
}

type SessionContextValue = {
  session: SessionState
  openProject: (path: string) => void
  closeProject: () => void
  startEditing: (songId: string) => void
  startPerforming: (setlistId: string) => void
  stopActivity: () => void
  advance: () => void
  previous: () => void
  goTo: (index: number) => void
  hasRestored: boolean
}

const defaultPlayback: SessionPlayback = {
  isPlaying: false,
  playheadPosition: 0,
  cursorPosition: 0,
  masterVolume: 1,
}

const defaultSession: SessionState = {
  hasProject: false,
  project: null,
  activity: 'none',
  editing: null,
  performing: null,
  playback: defaultPlayback,
}

const SessionContext = createContext<SessionContextValue>({
  session: defaultSession,
  openProject: () => undefined,
  closeProject: () => undefined,
  startEditing: () => undefined,
  startPerforming: () => undefined,
  stopActivity: () => undefined,
  advance: () => undefined,
  previous: () => undefined,
  goTo: () => undefined,
  hasRestored: false,
})

type SessionProviderProps = {
  children: React.ReactNode
}

export function SessionProvider({ children }: Readonly<SessionProviderProps>) {
  const { ws, isConnected, send } = useWebSocket()
  const navigate = useNavigate()
  const [session, setSession] = useState<SessionState>(defaultSession)
  const [hasRestored, setHasRestored] = useState(false)
  const hasRestoredRef = useRef(false)

  useEffect(() => {
    if (!ws || !isConnected) return
    ws.addEventListener('message', onMessage)
    send({ action: 'session.get' })
    return () => ws.removeEventListener('message', onMessage)
  }, [ws, isConnected])

  function onMessage(ev: MessageEvent<unknown>) {
    if (typeof ev.data !== 'string') return
    const data = JSON.parse(ev.data) as Record<string, unknown>
    if (
      data.event !== 'session.changed' &&
      data.event !== 'session.currentState'
    )
      return

    const playbackData = data.playback as Record<string, unknown> | undefined

    const next: SessionState = {
      hasProject: (data.hasProject as boolean) ?? false,
      project: (data.project as Project) ?? null,
      activity: (data.activity as SessionActivity) ?? 'none',
      editing: (data.editing as SessionEditing) ?? null,
      performing: (data.performing as SessionPerforming) ?? null,
      playback: {
        isPlaying: (playbackData?.isPlaying as boolean) ?? false,
        playheadPosition: (playbackData?.playheadPosition as number) ?? 0,
        cursorPosition: (playbackData?.cursorPosition as number) ?? 0,
        masterVolume: (playbackData?.masterVolume as number) ?? 1,
      },
    }
    setSession(next)

    // Route restoration (only once on first load)
    if (!hasRestoredRef.current && next.hasProject) {
      hasRestoredRef.current = true
      setHasRestored(true)
      if (next.activity === 'performing') {
        navigate('/live')
      } else if (next.activity === 'editing' && next.editing?.songId) {
        navigate(`/edit/${next.editing.songId}`)
      }
      return
    }

    // Navigate based on activity changes after route restoration
    if (hasRestoredRef.current && data.event === 'session.changed') {
      if (next.activity === 'performing') {
        navigate('/live')
      } else if (next.activity === 'editing' && next.editing?.songId) {
        navigate(`/edit/${next.editing.songId}`)
      } else if (next.activity === 'none' && next.hasProject) {
        navigate('/')
      }
    }
  }

  const openProject = useCallback(
    (path: string) => send({ action: 'session.openProject', path }),
    [send],
  )
  const closeProject = useCallback(
    () => send({ action: 'session.closeProject' }),
    [send],
  )
  const startEditing = useCallback(
    (songId: string) => send({ action: 'session.startEditing', songId }),
    [send],
  )
  const startPerforming = useCallback(
    (setlistId: string) =>
      send({ action: 'session.startPerforming', setlistId }),
    [send],
  )
  const stopActivity = useCallback(
    () => send({ action: 'session.stopActivity' }),
    [send],
  )
  const advance = useCallback(
    () => send({ action: 'session.advance' }),
    [send],
  )
  const previous = useCallback(
    () => send({ action: 'session.previous' }),
    [send],
  )
  const goTo = useCallback(
    (index: number) => send({ action: 'session.goTo', index }),
    [send],
  )

  const value: SessionContextValue = {
    session,
    openProject,
    closeProject,
    startEditing,
    startPerforming,
    stopActivity,
    advance,
    previous,
    goTo,
    hasRestored,
  }

  return (
    <SessionContext.Provider value={value}>{children}</SessionContext.Provider>
  )
}

export const useSession = () => {
  const context = useContext(SessionContext)
  if (context === undefined)
    throw new Error('useSession must be used within a SessionProvider')
  return context
}
