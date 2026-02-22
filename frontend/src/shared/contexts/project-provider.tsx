import {
  createContext,
  useContext,
  useEffect,
  useState,
  useCallback,
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
  project: Project | undefined | null
  setProject: (project: Project) => void
  playheadPos: number
  cursorPos: number
  playing: boolean
  activeSong: Song | undefined
  trackViews: TrackView[]
  availableInputs: AudioInput[]
  setTrackInput: (
    trackId: string,
    inputChannel: number,
    stereo: boolean,
  ) => void
  setTrackMonitoring: (trackId: string, monitoring: boolean) => void
  fetchAudioInputs: () => void
}

const initialState: ProjectProviderState = {
  project: undefined,
  setProject: () => null,
  playheadPos: 0,
  cursorPos: 0,
  playing: false,
  activeSong: undefined,
  trackViews: [],
  availableInputs: [],
  setTrackInput: () => null,
  setTrackMonitoring: () => null,
  fetchAudioInputs: () => null,
}

const ProjectProviderContext = createContext<ProjectProviderState>(initialState)

export function ProjectProvider({
  children,
  ...props
}: Readonly<ProjectProviderProps>) {
  const { ws, isConnected, send } = useWebSocket()
  const [project, setProject] = useState<Project | null>()
  const [activeSong, setActiveSong] = useState<Song>()
  const [playheadPos, setPlayheadPos] = useState<number>(0)
  const [cursorPos, setCursorPos] = useState<number>(0)
  const [playing, setPlaying] = useState<boolean>(false)
  const [trackViews, setTrackViews] = useState<TrackView[]>([])
  const [availableInputs, setAvailableInputs] = useState<AudioInput[]>([])

  const fetchAudioInputs = useCallback(() => {
    if (ws && isConnected) {
      send({ action: 'audio.listInputs' })
    }
  }, [ws, isConnected, send])

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

        break
      }
      case 'project.loaded': {
        if (data.project !== undefined) setProject(data.project)
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
      case 'transport.play': {
        setPlaying(true)
        break
      }
      case 'transport.pause': {
        setPlaying(false)
        break
      }
      case 'transport.stop': {
        setPlaying(false)
        break
      }
      case 'audio.inputsList': {
        if (data.inputs !== undefined) setAvailableInputs(data.inputs)
        break
      }
      case 'track.inputChanged': {
        setActiveSong((prev) => {
          if (!prev) return prev
          return {
            ...prev,
            tracks: prev.tracks.map((t) =>
              t.id === data.trackId
                ? {
                    ...t,
                    inputChannel: data.inputChannel,
                    inputStereo: data.stereo,
                  }
                : t,
            ),
          }
        })
        break
      }
      case 'track.monitoringChanged': {
        setActiveSong((prev) => {
          if (!prev) return prev
          return {
            ...prev,
            tracks: prev.tracks.map((t) =>
              t.id === data.trackId ? { ...t, monitoring: data.monitoring } : t,
            ),
          }
        })
        break
      }
    }
  }

  useEffect(() => {
    if (!activeSong) return
    const trackViews = activeSong.tracks.map((t, i) => {
      const colorIndex = (i % 8) + 1
      const tv = {
        track: t,
        fillColor: `--track-${colorIndex}-fill`,
        strokeColor: `--track-${colorIndex}-stroke`,
        height: 100,
      } satisfies TrackView
      return tv
    })
    setTrackViews(trackViews)
  }, [activeSong])

  const value = {
    project,
    setProject: (project: Project) => {
      setProject(project)
    },
    playheadPos,
    cursorPos,
    playing,
    activeSong,
    trackViews,
    availableInputs,
    setTrackInput,
    setTrackMonitoring,
    fetchAudioInputs,
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
