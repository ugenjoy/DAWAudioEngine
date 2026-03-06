import { useState } from 'react'
import { useProject } from '@/shared/contexts/project-provider'
import { Button } from '@/shared/shadcn/components/button'
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
} from '@/shared/shadcn/components/dialog'
import { Input } from '@/shared/shadcn/components/input'
import { IconPlus } from '@tabler/icons-react'

export function NewSongDialog() {
  const { createSong } = useProject()
  const [open, setOpen] = useState(false)
  const [name, setName] = useState('')

  function handleSubmit() {
    const trimmed = name.trim()
    if (!trimmed) return
    createSong(trimmed)
    setName('')
    setOpen(false)
  }

  return (
    <Dialog
      open={open}
      onOpenChange={(v) => {
        setOpen(v)
        if (!v) setName('')
      }}
    >
      <DialogTrigger asChild>
        <Button variant="ghost" size="icon-sm" title="New song">
          <IconPlus size={16} />
        </Button>
      </DialogTrigger>
      <DialogContent>
        <DialogHeader>
          <DialogTitle>New Song</DialogTitle>
        </DialogHeader>
        <form
          onSubmit={(e) => {
            e.preventDefault()
            handleSubmit()
          }}
          className="flex flex-col gap-4"
        >
          <Input
            placeholder="Song name"
            value={name}
            onChange={(e) => setName(e.target.value)}
            autoFocus
          />
          <Button type="submit" disabled={!name.trim()}>
            Create
          </Button>
        </form>
      </DialogContent>
    </Dialog>
  )
}
