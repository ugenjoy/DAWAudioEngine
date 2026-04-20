import {
  createContext,
  useContext,
  useEffect,
  useState,
  useCallback,
  useRef,
} from 'react'
import { useNavigate } from 'react-router'
import { Project } from '../models/project'
import { useWebSocket } from './websocket-provider'
import { Song } from '../models/song'
import { Loop } from '../models/loop'
import { Track } from '../models/track'
import { AudioInput } from '../models/audio-input'

export interface TrackView {
  track: Track
  height: number
  fillColor: string
  strokeColor: string
}

type ProjectProviderProps = {
  children: React.ReactNode
  defaultProject?: Project
  storageKey?: string
}

type ProjectProviderState = {
  project: Project | null
  loadProject: (path: string) => void
  createProject: (name: string) => void
  isLoading: boolean
  songLoading: boolean
  playheadPosRef: React.RefObject<number>
  cursorPosRef: React.RefObject<number>
  playheadUpdateRef: React.RefObject<number>
  playing: boolean
  masterVolume: number
  setMasterVolume: (volume: number) => void
  activeSong: Song | undefined
  trackViews: TrackView[]
  availableInputs: AudioInput[]
  setTrackInput: (
    trackId: string,
    inputChannel: number,
    stereo: boolean,
  ) => void
  setTrackMonitoring: (trackId: string, monitoring: boolean) => void
  setTrackMute: (trackId: string, mute: boolean) => void
  setTrackSolo: (trackId: string, solo: boolean) => void
  setTrackVolume: (trackId: string, volume: number) => void
  addTrack: (name?: string) => void
  removeTrack: (trackId: string) => void
  renameTrack: (trackId: string, name: string) => void
  reorderTrack: (trackId: string, index: number) => void
  setTrackColor: (trackId: string, color: number) => void
  setTrackHeight: (trackId: string, height: number) => void
  isDirty: boolean
  saveProject: () => void
  setTempo: (tempo: number) => void
  setMetronomeMute: (mute: boolean) => void
  createSong: (name: string) => void
  renameSong: (uuid: string, name: string) => void
  reorderSong: (uuid: string, index: number) => void
  loadSong: (uuid: string) => void
  fetchAudioInputs: () => void
  trackLevelsRef: React.RefObject<Record<string, number>>
  loops: Loop[]
  activeLoop: Loop | null
  addLoop: (start: number, end: number) => void
  removeLoop: (loopId: string) => void
  updateLoop: (loopId: string, start: number, end: number) => void
  cancelLoop: () => void
  exitLoop: () => void
}

const initialState: ProjectProviderState = {
  project: null,
  loadProject: () => null,
  createProject: () => null,
  isLoading: false,
  songLoading: false,
  playheadPosRef: { current: 0 },
  cursorPosRef: { current: 0 },
  playheadUpdateRef: { current: 0 },
  playing: false,
  masterVolume: 1,
  setMasterVolume: () => null,
  activeSong: undefined,
  trackViews: [],
  availableInputs: [],
  setTrackInput: () => null,
  setTrackMonitoring: () => null,
  setTrackMute: () => null,
  setTrackSolo: () => null,
  setTrackVolume: () => null,
  addTrack: () => null,
  removeTrack: () => null,
  renameTrack: () => null,
  reorderTrack: () => null,
  setTrackColor: () => null,
  setTrackHeight: () => null,
  isDirty: false,
  saveProject: () => null,
  setTempo: () => null,
  setMetronomeMute: () => null,
  createSong: () => null,
  renameSong: () => null,
  reorderSong: () => null,
  loadSong: () => null,
  fetchAudioInputs: () => null,
  trackLevelsRef: { current: {} },
  loops: [],
  activeLoop: null,
  addLoop: () => null,
  removeLoop: () => null,
  updateLoop: () => null,
  cancelLoop: () => null,
  exitLoop: () => null,
}

const ProjectProviderContext = createContext<ProjectProviderState>(initialState)

export function ProjectProvider({
  children,
  ...props
}: Readonly<ProjectProviderProps>) {
  const { ws, isConnected, send } = useWebSocket()
  const navigate = useNavigate()
  const hasRestoredRef = useRef(false)
  const [project, setProject] = useState<Project | null>(null)
  const [isLoading, setIsLoading] = useState<boolean>(false)
  const [activeSong, setActiveSong] = useState<Song>()
  const playheadPosRef = useRef<number>(0)
  const cursorPosRef = useRef<number>(0)
  const playheadUpdateRef = useRef<number>(0)
  const [playing, setPlaying] = useState<boolean>(false)
  const [trackViews, setTrackViews] = useState<TrackView[]>([])
  const [availableInputs, setAvailableInputs] = useState<AudioInput[]>([])
  const [isDirty, setIsDirty] = useState<boolean>(false)
  const [masterVolume, setMasterVolume] = useState<number>(1)
  const [songLoading, setSongLoading] = useState<boolean>(false)
  const trackLevelsRef = useRef<Record<string, number>>({})
  const [loops, setLoops] = useState<Loop[]>([])
  const [activeLoop, setActiveLoop] = useState<Loop | null>(null)

  const fetchAudioInputs = useCallback(() => {
    if (ws && isConnected) {
      send({ action: 'audio.listInputs' })
    }
  }, [ws, isConnected, send])

  const loadProject = useCallback(
    (path: string) => {
      send({ action: 'project.load', path: path })
      setIsLoading(true)
    },
    [send],
  )

  const createProject = useCallback(
    (name: string) => {
      send({ action: 'project.create', name })
      setIsLoading(true)
    },
    [send],
  )

  const setTrackInput = useCallback(
    (trackId: string, inputChannel: number, stereo: boolean) => {
      send({ action: 'track.setInput', trackId, inputChannel, stereo })
    },
    [send],
  )

  const setTrackMonitoring = useCallback(
    (trackId: string, monitoring: boolean) => {
      send({ action: 'track.setMonitoring', trackId, monitoring })
    },
    [send],
  )

  const setTrackMute = useCallback(
    (trackId: string, mute: boolean) => {
      send({ action: 'track.setMute', trackId, mute })
    },
    [send],
  )

  const setTrackSolo = useCallback(
    (trackId: string, solo: boolean) => {
      send({ action: 'track.setSolo', trackId, solo })
    },
    [send],
  )

  const setTrackVolume = useCallback(
    (trackId: string, volume: number) => {
      send({ action: 'track.setVolume', trackId, volume })
    },
    [send],
  )

  const addTrack = useCallback(
    (name?: string) => {
      send({ action: 'track.add', ...(name ? { name } : {}) })
    },
    [send],
  )

  const removeTrack = useCallback(
    (trackId: string) => {
      send({ action: 'track.remove', trackId })
    },
    [send],
  )

  const renameTrack = useCallback(
    (trackId: string, name: string) => {
      updateTrack(trackId, { name })
      send({ action: 'track.rename', trackId, name })
    },
    [send],
  )

  const reorderTrack = useCallback(
    (trackId: string, index: number) => {
      setActiveSong((prev) => {
        if (!prev) return prev
        const tracks = [...prev.tracks]
        const oldIndex = tracks.findIndex((t) => t.id === trackId)
        if (oldIndex < 0) return prev
        const [moved] = tracks.splice(oldIndex, 1)
        const clampedIndex = Math.max(0, Math.min(tracks.length, index))
        tracks.splice(clampedIndex, 0, moved)
        return { ...prev, tracks }
      })
      setIsDirty(true)
      send({ action: 'track.reorder', trackId, index })
    },
    [send],
  )

  const setTrackColor = useCallback(
    (trackId: string, color: number) => {
      updateTrack(trackId, { color })
      send({ action: 'track.setColor', trackId, color })
    },
    [send],
  )

  const saveProject = useCallback(() => {
    if (project?.path) {
      send({ action: 'project.save', path: project.path })
    }
  }, [send, project])

  const setTempo = useCallback(
    (tempo: number) => {
      send({ action: 'song.setTempo', tempo })
    },
    [send],
  )

  const setMetronomeMute = useCallback(
    (mute: boolean) => {
      send({ action: 'song.setMetronomeMute', mute })
    },
    [send],
  )

  const createSong = useCallback(
    (name: string) => {
      send({ action: 'song.create', name })
    },
    [send],
  )

  const renameSong = useCallback(
    (uuid: string, name: string) => {
      setProject((prev) => {
        if (!prev) return prev
        return {
          ...prev,
          songs: prev.songs.map((s) => (s.id === uuid ? { ...s, name } : s)),
        }
      })
      if (activeSong?.id === uuid) {
        setActiveSong((prev) => (prev ? { ...prev, name } : prev))
      }
      setIsDirty(true)
      send({ action: 'song.rename', uuid, name })
    },
    [send, activeSong],
  )

  const reorderSong = useCallback(
    (uuid: string, index: number) => {
      setProject((prev) => {
        if (!prev) return prev
        const songs = [...prev.songs]
        const oldIndex = songs.findIndex((s) => s.id === uuid)
        if (oldIndex < 0) return prev
        const [moved] = songs.splice(oldIndex, 1)
        const clamped = Math.max(0, Math.min(songs.length, index))
        songs.splice(clamped, 0, moved)
        return { ...prev, songs }
      })
      setIsDirty(true)
      send({ action: 'song.reorder', uuid, index })
    },
    [send],
  )

  const loadSong = useCallback(
    (uuid: string) => {
      send({ action: 'project.loadSong', uuid })
    },
    [send],
  )

  const addLoop = useCallback(
    (start: number, end: number) => {
      if (!activeSong) return
      send({ action: 'loop.add', start, end })
    },
    [activeSong, send],
  )

  const removeLoop = useCallback(
    (loopId: string) => {
      send({ action: 'loop.remove', loopId })
    },
    [send],
  )

  const updateLoop = useCallback(
    (loopId: string, start: number, end: number) => {
      send({ action: 'loop.update', loopId, start, end })
    },
    [send],
  )

  const cancelLoop = useCallback(() => {
    send({ action: 'loop.cancel' })
  }, [send])

  const exitLoop = useCallback(() => {
    send({ action: 'loop.exit' })
  }, [send])

  const setTrackHeight = useCallback((trackId: string, height: number) => {
    const clampedHeight = Math.max(60, height)
    setTrackViews((prev) =>
      prev.map((tv) =>
        tv.track.id === trackId ? { ...tv, height: clampedHeight } : tv,
      ),
    )
  }, [])

  const sendMasterVolume = useCallback(
    (volume: number) => {
      setMasterVolume(volume)
      send({ action: 'transport.setMasterVolume', volume })
    },
    [send],
  )

  useEffect(() => {
    if (ws && isConnected) {
      ws.addEventListener('message', onMessage)
      try {
        send({ action: 'project.getLoaded' })
        send({ action: 'audio.listInputs' })
      } catch (error) {
        console.error(error)
      }
      return () => ws.removeEventListener('message', onMessage)
    }
  }, [ws, isConnected])

  function updateTrack(trackId: string, updates: Partial<Track>) {
    setIsDirty(true)
    setActiveSong((prev) => {
      if (!prev) return prev
      return {
        ...prev,
        tracks: prev.tracks.map((t) =>
          t.id === trackId ? { ...t, ...updates } : t,
        ),
      }
    })
  }

  function onMessage(ev: MessageEvent<unknown>) {
    if (typeof ev.data !== 'string') return
    const data = JSON.parse(ev.data)
    switch (data.event) {
      case 'project.currentLoaded': {
        if (data.project !== undefined) setProject(data.project)
        if (data.activeSong !== undefined) {
          setActiveSong(data.activeSong)
          setLoops(data.activeSong?.loops ?? [])
          setActiveLoop(null)
        }
        if (data.playheadPosition !== undefined) {
          playheadPosRef.current = data.playheadPosition
          playheadUpdateRef.current++
        }
        if (data.cursorPosition !== undefined)
          cursorPosRef.current = data.cursorPosition
        if (data.isPlaying !== undefined) setPlaying(data.isPlaying)
        if (data.masterVolume !== undefined) setMasterVolume(data.masterVolume)

        // Restore route on first load after refresh
        if (!hasRestoredRef.current && data.hasProject) {
          hasRestoredRef.current = true
          if (data.mode === 'live' && data.setlist) {
            navigate('/live')
          } else if (data.mode === 'edit' && data.activeSong?.id) {
            navigate(`/edit/${data.activeSong.id}`)
          }
        }
        break
      }
      case 'project.loaded':
      case 'project.created': {
        if (data.project !== undefined) setProject(data.project)
        setActiveSong(undefined)
        setLoops([])
        setActiveLoop(null)
        setIsDirty(false)
        setIsLoading(false)
        break
      }
      case 'project.saved': {
        setIsDirty(false)
        break
      }
      case 'song.loading': {
        setSongLoading(true)
        break
      }
      case 'song.loaded': {
        setSongLoading(false)
        if (data.song !== undefined) {
          const songData = data.song as Song
          setActiveSong(songData)
          setLoops(songData.loops ?? [])
          setActiveLoop(null)
        }
        break
      }
      case 'song.unloaded': {
        setLoops([])
        setActiveLoop(null)
        break
      }
      case 'song.endPositionUpdated': {
        if (data.songId) {
          setIsDirty(true)
          setActiveSong((prev) => {
            if (!prev || prev.id !== data.songId) return prev
            return { ...prev, endPosition: data.endPosition ?? undefined }
          })
        }
        break
      }
      case 'transport.playheadPosition': {
        if (data.position !== undefined) {
          playheadPosRef.current = Number(data.position.toFixed(2))
          playheadUpdateRef.current++
        }
        break
      }
      case 'transport.cursorPosition': {
        if (data.position !== undefined) {
          cursorPosRef.current = Number(data.position.toFixed(2))
        }
        break
      }
      case 'track.levels': {
        if (data.levels !== undefined) trackLevelsRef.current = data.levels
        break
      }
      case 'transport.play': {
        setPlaying(true)
        break
      }
      case 'transport.pause': {
        setPlaying(false)
        trackLevelsRef.current = {}
        break
      }
      case 'transport.stop': {
        setPlaying(false)
        trackLevelsRef.current = {}
        break
      }
      case 'audio.inputsList': {
        if (data.inputs !== undefined) setAvailableInputs(data.inputs)
        break
      }
      case 'song.tempoChanged': {
        setIsDirty(true)
        setActiveSong((prev) => {
          if (!prev) return prev
          return { ...prev, tempo: data.tempo }
        })
        break
      }
      case 'song.metronomeMuteChanged': {
        setIsDirty(true)
        setActiveSong((prev) => {
          if (!prev) return prev
          return { ...prev, metronomeMute: data.mute }
        })
        break
      }
      case 'track.inputChanged': {
        updateTrack(data.trackId, {
          inputChannel: data.inputChannel,
          inputStereo: data.stereo,
        })
        break
      }
      case 'track.monitoringChanged': {
        updateTrack(data.trackId, { monitoring: data.monitoring })
        break
      }
      case 'track.muteChanged': {
        updateTrack(data.trackId, { mute: data.mute })
        break
      }
      case 'track.soloChanged': {
        updateTrack(data.trackId, { solo: data.solo })
        break
      }
      case 'track.volumeChanged': {
        updateTrack(data.trackId, { volume: data.volume })
        break
      }
      case 'track.colorChanged': {
        updateTrack(data.trackId, { color: data.color })
        break
      }
      case 'track.listUpdated': {
        setIsDirty(true)
        if (data.tracks !== undefined) {
          setActiveSong((prev) => {
            if (!prev) return prev
            return { ...prev, tracks: data.tracks }
          })
        }
        break
      }
      case 'transport.masterVolume': {
        if (data.volume !== undefined) setMasterVolume(data.volume)
        break
      }
      case 'project.songsUpdated': {
        setIsDirty(true)
        if (data.songs !== undefined) {
          setProject((prev) => (prev ? { ...prev, songs: data.songs } : prev))
        }
        break
      }
      case 'event.listUpdated': {
        setIsDirty(true)
        break
      }
      case 'marker.listUpdated': {
        setIsDirty(true)
        break
      }
      case 'loop.listUpdated': {
        setIsDirty(true)
        const { loops: newLoops } = data as { loops: Loop[] }
        setLoops(newLoops ?? [])
        break
      }
      case 'loop.activated': {
        const { loop } = data as { loop: Loop }
        setActiveLoop(loop)
        break
      }
      case 'loop.deactivated': {
        setActiveLoop(null)
        break
      }
    }
  }

  useEffect(() => {
    if (!activeSong) return
    setTrackViews((prev) => {
      const heightMap = new Map(prev.map((tv) => [tv.track.id, tv.height]))
      return activeSong.tracks.map((t) => {
        const colorIndex = t.color ?? 1
        return {
          track: t,
          fillColor: `--track-${colorIndex}-fill`,
          strokeColor: `--track-${colorIndex}-stroke`,
          height: heightMap.get(t.id) ?? 100,
        } satisfies TrackView
      })
    })
  }, [activeSong])

  const value = {
    project,
    loadProject,
    createProject,
    isLoading,
    songLoading,
    playheadPosRef,
    cursorPosRef,
    playheadUpdateRef,
    playing,
    masterVolume,
    setMasterVolume: sendMasterVolume,
    activeSong,
    trackViews,
    availableInputs,
    setTrackInput,
    setTrackMonitoring,
    setTrackMute,
    setTrackSolo,
    setTrackVolume,
    addTrack,
    removeTrack,
    renameTrack,
    reorderTrack,
    setTrackColor,
    setTrackHeight,
    isDirty,
    saveProject,
    createSong,
    renameSong,
    reorderSong,
    loadSong,
    setTempo,
    setMetronomeMute,
    fetchAudioInputs,
    trackLevelsRef,
    loops,
    activeLoop,
    addLoop,
    removeLoop,
    updateLoop,
    cancelLoop,
    exitLoop,
  }

  return (
    <ProjectProviderContext.Provider {...props} value={value}>
      {children}
    </ProjectProviderContext.Provider>
  )
}

export const useProject = () => {
  const context = useContext(ProjectProviderContext)
  if (context === undefined)
    throw new Error('useProject must be used within a ProjectProvider')
  return context
}
