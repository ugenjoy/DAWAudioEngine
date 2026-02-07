import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { Input } from '@/shared/shadcn/components/input'
import { useEffect } from 'react'

function ProjectsHome() {
  const { ws, send } = useWebSocket()

  useEffect(() => {
    if (ws) {
      try {
        send({
          action: 'project.getLoaded',
          path: '/home/ugo/dev/daw/backend/assets/demo.dawproj',
        })
      } catch (error) {
        console.error(error)
      }
    }
  }, [ws])

  return (
    <div>
      <h2>Projects</h2>
      <div>
        <Input type="file" multiple />
      </div>
    </div>
  )
}

export default ProjectsHome
