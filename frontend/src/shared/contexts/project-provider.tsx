import { createContext, useContext, useEffect, useState } from 'react'
import { Project } from '../models/project'
import { useWebSocket } from './websocket-provider'

type ProjectProviderProps = {
  children: React.ReactNode
  defaultProject?: Project
  storageKey?: string
}

type ProjectProviderState = {
  project: Project | undefined
  setProject: (project: Project) => void
}

const initialState: ProjectProviderState = {
  project: undefined,
  setProject: () => null,
}

const ProjectProviderContext = createContext<ProjectProviderState>(initialState)

export function ProjectProvider({
  children,
  ...props
}: Readonly<ProjectProviderProps>) {
  const { ws, isConnected, send } = useWebSocket()
  const [project, setProject] = useState<Project>()

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
    if (data.event === 'project.currentLoaded') {
      if (data.hasProject) {
        const project = data.project
        setProject(project)
      }
    } else if (data.event === 'project.loaded') {
      const project = data.project
      setProject(project)
    }
  }

  const value = {
    project,
    setProject: (project: Project) => {
      setProject(project)
    },
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
