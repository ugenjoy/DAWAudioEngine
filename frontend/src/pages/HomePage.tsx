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
    setProjectsDialogOpen(!project)
  }, [project])

  return (
    <>
      <main className="flex flex-col h-screen">
        <Navbar />
        {isConnected && (
          <div className="flex-1 overflow-auto">{project && <Sequencer />}</div>
        )}
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
