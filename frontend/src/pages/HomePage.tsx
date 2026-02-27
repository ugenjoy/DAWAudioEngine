import Navbar from '@/features/navbar/components/Navbar'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { useProject } from '../shared/contexts/project-provider'
import ProjectsDialog from '@/features/projects/components/ProjectsDialog'
import { useEffect, useState } from 'react'
import Sequencer from '@/features/sequencer/components/Sequencer'
import { Navigate } from 'react-router'
import { IconLoader } from '@tabler/icons-react'

function HomePage() {
  const { isConnected, autoConnect } = useWebSocket()
  const { project } = useProject()

  const [projectsDialogOpen, setProjectsDialogOpen] = useState<boolean>(false)

  useEffect(() => {
    setProjectsDialogOpen(isConnected && project === null)
  }, [project, isConnected])

  if (isConnected) {
    return (
      <>
        <main className="flex flex-col h-screen w-screen">
          <Navbar
            onOpenProjectDialog={() => setProjectsDialogOpen(true)}
            onOpenConnectionDialog={() => null}
          />
          <div className="flex-1 overflow-auto">
            <Sequencer />
          </div>
        </main>
        <ProjectsDialog
          open={projectsDialogOpen}
          setOpen={setProjectsDialogOpen}
        />
      </>
    )
  } else if (autoConnect) {
    return (
      <main className="flex flex-col h-screen w-screen items-center justify-center gap-4">
        <IconLoader className="animate-spin size-8" />
        <p className="text-sm text-muted-foreground">
          Connecting to backend...
        </p>
      </main>
    )
  } else {
    return <Navigate to="/connect" />
  }
}

export default HomePage
