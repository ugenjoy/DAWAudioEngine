import {
  createContext,
  useContext,
  useEffect,
  useState,
  useCallback,
  useRef,
} from 'react'
import { Project } from '../models/project'
import { useWebSocket } from './websocket-provider'
import { Song } from '../models/song'
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
  isLoading: boolean
  playheadPos: number
  cursorPos: number
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
  fetchAudioInputs: () => void
  trackLevelsRef: React.RefObject<Record<string, number>>
}

const initialState: ProjectProviderState = {
  project: null,
  loadProject: () => null,
  isLoading: false,
  playheadPos: 0,
  cursorPos: 0,
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
  fetchAudioInputs: () => null,
  trackLevelsRef: { current: {} },
}

const ProjectProviderContext = createContext<ProjectProviderState>(initialState)

export function ProjectProvider({
  children,
  ...props
}: Readonly<ProjectProviderProps>) {
  const { ws, isConnected, send } = useWebSocket()
  const [project, setProject] = useState<Project | null>(null)
  const [isLoading, setIsLoading] = useState<boolean>(false)
  const [activeSong, setActiveSong] = useState<Song>()
  const [playheadPos, setPlayheadPos] = useState<number>(0)
  const [cursorPos, setCursorPos] = useState<number>(0)
  const [playing, setPlaying] = useState<boolean>(false)
  const [trackViews, setTrackViews] = useState<TrackView[]>([])
  const [availableInputs, setAvailableInputs] = useState<AudioInput[]>([])
  const [isDirty, setIsDirty] = useState<boolean>(false)
  const [masterVolume, setMasterVolume] = useState<number>(1)
  const trackLevelsRef = useRef<Record<string, number>>({})

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

  const setTrackHeight = useCallback((trackId: string, height: number) => {
    const clampedHeight = Math.max(60, height)
    setTrackViews(prev => prev.map(tv =>
      tv.track.id === trackId ? { ...tv, height: clampedHeight } : tv
    ))
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
        if (data.activeSong !== undefined) setActiveSong(data.activeSong)
        if (data.playheadPosition !== undefined)
          setPlayheadPos(data.playheadPosition)
        if (data.cursorPosition !== undefined) setCursorPos(data.cursorPosition)
        if (data.isPlaying !== undefined) setPlaying(data.isPlaying)
        if (data.masterVolume !== undefined) setMasterVolume(data.masterVolume)
        break
      }
      case 'project.loaded': {
        if (data.project !== undefined) setProject(data.project)
        setIsDirty(false)
        setIsLoading(false)
        break
      }
      case 'project.saved': {
        setIsDirty(false)
        break
      }
      case 'song.loaded': {
        if (data.song !== undefined) setActiveSong(data.song)
        break
      }
      case 'transport.playheadPosition': {
        if (data.position !== undefined)
          setPlayheadPos(Number(data.position.toFixed(2)))
        break
      }
      case 'transport.cursorPosition': {
        if (data.position !== undefined) {
          setCursorPos(Number(data.position.toFixed(2)))
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
      case 'event.listUpdated': {
        setIsDirty(true)
        break
      }
    }
  }

  useEffect(() => {
    if (!activeSong) return
    setTrackViews(prev => {
      const heightMap = new Map(prev.map(tv => [tv.track.id, tv.height]))
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
    isLoading,
    playheadPos,
    cursorPos,
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
    setTempo,
    setMetronomeMute,
    fetchAudioInputs,
    trackLevelsRef,
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
