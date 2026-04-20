import { useState } from 'react'
import { useProject } from '@/shared/contexts/project-provider'
import { useProjects } from '@/shared/contexts/projects-provider'
import { Card } from '@/shared/shadcn/components/card'
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
} from '@/shared/shadcn/components/dialog'
import { Button } from '@/shared/shadcn/components/button'
import { Input } from '@/shared/shadcn/components/input'
import { IconLoader, IconPlus } from '@tabler/icons-react'

interface ProjectDialogProps {
  open: boolean
  setOpen: (value: boolean) => void
}

function ProjectsDialog({ open, setOpen }: Readonly<ProjectDialogProps>) {
  const { projects } = useProjects()
  const { loadProject, createProject, isLoading } = useProject()
  const [newName, setNewName] = useState('')

  function handleCreate() {
    const trimmed = newName.trim()
    if (!trimmed || isLoading) return
    createProject(trimmed)
    setNewName('')
    setOpen(false)
  }

  return (
    <Dialog
      open={open}
      onOpenChange={(value) => {
        setOpen(value)
      }}
    >
      <DialogContent>
        <DialogHeader>
          <DialogTitle>Projects</DialogTitle>
        </DialogHeader>

        {isLoading && (
          <div className="absolute left-0 right-0 top-0 z-10 bottom-0 bg-background/65 flex justify-center items-center">
            <IconLoader className="animate-spin" size={32} />
          </div>
        )}

        <div className="flex gap-2">
          <Input
            placeholder="New project name"
            value={newName}
            onChange={(e) => setNewName(e.target.value)}
            onKeyDown={(e) => e.key === 'Enter' && handleCreate()}
          />
          <Button onClick={handleCreate} disabled={!newName.trim() || isLoading}>
            <IconPlus className="size-4 mr-1" />
            Create
          </Button>
        </div>

        {projects.length > 0 && (
          <div className="flex flex-row flex-wrap justify-start gap-2">
            {projects.map((p) => (
              <Card
                key={p.id}
                className="flex flex-col p-4 gap-2 w-fit hover:bg-accent hover:cursor-pointer relative"
                onClick={() => {
                  if (!isLoading) {
                    loadProject(p.path)
                    setOpen(false)
                  }
                }}
              >
                <h3 className="text-lg font-semibold">{p.name}</h3>
                <p>songs: {p.songs.length}</p>
                <p>
                  last modified: {new Date(p.lastModified).toLocaleDateString()}
                </p>
              </Card>
            ))}
          </div>
        )}
      </DialogContent>
    </Dialog>
  )
}

export default ProjectsDialog
