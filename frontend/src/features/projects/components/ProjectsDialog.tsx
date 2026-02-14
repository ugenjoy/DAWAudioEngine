import { useProjects } from '@/shared/contexts/projects-provider'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { Card } from '@/shared/shadcn/components/card'
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
} from '@/shared/shadcn/components/dialog'

interface ProjectDialogProps {
  open: boolean
  setOpen: (value: boolean) => void
}

function ProjectsDialog({ open, setOpen }: Readonly<ProjectDialogProps>) {
  const { projects } = useProjects()
  const { send } = useWebSocket()

  function loadProject(path: string) {
    send({
      action: 'project.load',
      path,
    })
  }

  return (
    <Dialog open={open} onOpenChange={setOpen}>
      <DialogContent>
        <DialogHeader>
          <DialogTitle>Load a project</DialogTitle>
        </DialogHeader>
        <div className="flex flex-row justify-start gap-2">
          {projects.map((p) => {
            return (
              <Card
                key={p.id}
                className="flex flex-col p-4 gap-2 w-fit hover:bg-accent hover:cursor-pointer"
                onClick={() => loadProject(p.path)}
              >
                <h3 className="text-lg font-semibold">{p.name}</h3>
                <p>songs: {p.songs.length}</p>
                <p>
                  last modified: {new Date(p.lastModified).toLocaleDateString()}
                </p>
              </Card>
            )
          })}
        </div>
      </DialogContent>
    </Dialog>
  )
}

export default ProjectsDialog
