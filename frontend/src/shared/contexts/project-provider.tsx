import { createContext, useContext, useEffect, useState } from 'react'
import { Project } from '../models/project'
import { useWebSocket } from './websocket-provider'
import { Song } from '../models/song'

type ProjectProviderProps = {
  children: React.ReactNode
  defaultProject?: Project
  storageKey?: string
}

type ProjectProviderState = {
  project: Project | undefined | null
  setProject: (project: Project) => void
  activeSong: Song | undefined
  transportPos: number
}

const initialState: ProjectProviderState = {
  project: undefined,
  setProject: () => null,
  activeSong: undefined,
  transportPos: 0,
}

const ProjectProviderContext = createContext<ProjectProviderState>(initialState)

export function ProjectProvider({
  children,
  ...props
}: Readonly<ProjectProviderProps>) {
  const { ws, isConnected, send } = useWebSocket()
  const [project, setProject] = useState<Project | null>()
  const [activeSong, setActiveSong] = useState<Song>()
  const [transportPos, setTransportPos] = useState<number>(0)

  useEffect(() => {
    if (ws && isConnected) {
      ws.addEventListener('message', onMessage)
      try {
        send({
          action: 'project.getLoaded',
        })
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
        const project = data.project
        setProject(project)

        if (data.activeSong) {
          const song = data.activeSong
          setActiveSong(song)
        }
        break
      }
      case 'project.loaded': {
        const project = data.project
        setProject(project)
        break
      }
      case 'song.loaded': {
        const song = data.song
        setActiveSong(song)
        break
      }
      case 'transport.position': {
        const newPosition = Number(data.position.toFixed(2))
        setTransportPos(newPosition)
        break
      }
    }
  }

  const value = {
    project,
    setProject: (project: Project) => {
      setProject(project)
    },
    activeSong,
    transportPos,
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
