import { createContext, useContext, useState } from 'react'

type Track = {
  id: string
  name: string
  type: string
}

type Project = {
  id: string
  name: string
  tracks: Track[]
}

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
  const [project, setProject] = useState<Project>()

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
