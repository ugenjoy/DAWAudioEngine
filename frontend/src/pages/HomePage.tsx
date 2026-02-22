import Navbar from '@/features/navbar/components/Navbar'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { useProject } from '../shared/contexts/project-provider'
import ProjectsDialog from '@/features/projects/components/ProjectsDialog'
import ConnectionDialog from '@/features/websocket/components/ConnectionDialog'
import { useEffect, useState } from 'react'
import Sequencer from '@/features/sequencer/components/Sequencer'

function HomePage() {
  const { isConnected } = useWebSocket()
  const { project } = useProject()

  const [projectsDialogOpen, setProjectsDialogOpen] = useState<boolean>(false)
  const [connectionDialogOpen, setConnectionDialogOpen] =
    useState<boolean>(false)

  useEffect(() => {
    setConnectionDialogOpen(!isConnected)
  }, [isConnected])

  useEffect(() => {
    setProjectsDialogOpen(isConnected && project === null)
  }, [project, isConnected])

  return (
    <>
      <main className="flex flex-col h-screen w-screen">
        <Navbar onOpenProjectDialog={() => setProjectsDialogOpen(true)} />
        <div className="flex-1 overflow-auto">
          <Sequencer />
        </div>
      </main>
      <ProjectsDialog
        open={projectsDialogOpen}
        setOpen={setProjectsDialogOpen}
      />
      <ConnectionDialog
        open={connectionDialogOpen}
        setOpen={setConnectionDialogOpen}
      />
    </>
  )
}

export default HomePage
