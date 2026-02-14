import { createContext, useContext, useEffect, useState } from 'react'
import { Project } from '../models/project'
import { useWebSocket } from './websocket-provider'

type ProjectsProviderProps = {
  children: React.ReactNode
  storageKey?: string
}

type ProjectsProviderState = {
  projects: Project[]
}

const initialState: ProjectsProviderState = {
  projects: [],
}

const ProjectsProviderContext =
  createContext<ProjectsProviderState>(initialState)

export function ProjectsProvider({
  children,
  ...props
}: Readonly<ProjectsProviderProps>) {
  const { ws, isConnected, send } = useWebSocket()
  const [projects, setProjects] = useState<Project[]>([])

  useEffect(() => {
    if (ws && isConnected) {
      ws.addEventListener('message', onMessage)
      try {
        send({
          action: 'project.list',
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
    if (data.event === 'project.listed') {
      setProjects(data.projects)
    }
  }

  const value = {
    projects,
  }

  return (
    <ProjectsProviderContext.Provider {...props} value={value}>
      {children}
    </ProjectsProviderContext.Provider>
  )
}

export const useProjects = () => {
  const context = useContext(ProjectsProviderContext)
  if (context === undefined)
    throw new Error('useProject must be used within a ProjectsProvider')
  return context
}
