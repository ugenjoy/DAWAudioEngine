import { useProject } from '@/shared/contexts/project-provider'
import { useProjects } from '@/shared/contexts/projects-provider'
import { Card } from '@/shared/shadcn/components/card'
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
} from '@/shared/shadcn/components/dialog'
import { IconLoader } from '@tabler/icons-react'

interface ProjectDialogProps {
  open: boolean
  setOpen: (value: boolean) => void
}

function ProjectsDialog({ open, setOpen }: Readonly<ProjectDialogProps>) {
  const { projects } = useProjects()
  const { loadProject, isLoading } = useProject()

  return (
    <Dialog
      open={open}
      onOpenChange={(value) => {
        setOpen(value)
      }}
    >
      <DialogContent>
        <DialogHeader>
          <DialogTitle>Load a project</DialogTitle>
        </DialogHeader>
        <div className="flex flex-row justify-start gap-2 ">
          {isLoading && (
            <div className="absolute left-0 right-0 top-0 z-10 bottom-0 bg-background/65 flex justify-center items-center">
              <IconLoader className="animate-spin" size={32} />
            </div>
          )}
          {projects.map((p) => {
            return (
              <Card
                key={p.id}
                className="flex flex-col p-4 gap-2 w-fit hover:bg-accent hover:cursor-pointer relative"
                onClick={() => !isLoading && loadProject(p.path)}
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
