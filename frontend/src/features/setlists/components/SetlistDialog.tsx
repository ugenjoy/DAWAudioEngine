import { useEffect, useState } from 'react'
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
  DialogFooter,
} from '@/shared/shadcn/components/dialog'
import { Button } from '@/shared/shadcn/components/button'
import { Input } from '@/shared/shadcn/components/input'
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from '@/shared/shadcn/components/select'
import { useProject } from '@/shared/contexts/project-provider'
import { useSetlist } from '@/shared/contexts/setlist-provider'
import { Setlist, SetlistEntry, SetlistTransition } from '@/shared/models/setlist'
import { IconArrowUp, IconArrowDown, IconTrash } from '@tabler/icons-react'

interface Props {
  open: boolean
  setlist: Setlist | null
  onClose: () => void
}

export function SetlistDialog({ open, setlist, onClose }: Props) {
  const { project } = useProject()
  const { createSetlist, updateSetlist } = useSetlist()

  const [name, setName] = useState('')
  const [entries, setEntries] = useState<SetlistEntry[]>([])

  useEffect(() => {
    if (setlist) {
      setName(setlist.name)
      setEntries([...setlist.entries])
    } else {
      setName('')
      setEntries([])
    }
  }, [setlist, open])

  function addSong(songId: string) {
    setEntries([...entries, { songId, transition: 'stop' }])
  }

  function removeEntry(index: number) {
    setEntries(entries.filter((_, i) => i !== index))
  }

  function moveEntry(index: number, direction: -1 | 1) {
    const next = [...entries]
    const swap = index + direction
    if (swap < 0 || swap >= next.length) return
    ;[next[index], next[swap]] = [next[swap], next[index]]
    setEntries(next)
  }

  function setTransition(index: number, transition: SetlistTransition) {
    const next = [...entries]
    next[index] = { ...next[index], transition }
    setEntries(next)
  }

  function save() {
    if (!name.trim()) return
    if (setlist) {
      updateSetlist(setlist.id, name, entries, undefined)
    } else {
      createSetlist(name, entries, [])
    }
    onClose()
  }

  const availableSongs = project?.songs ?? []

  return (
    <Dialog open={open} onOpenChange={(o) => !o && onClose()}>
      <DialogContent className="max-w-xl">
        <DialogHeader>
          <DialogTitle>{setlist ? 'Edit setlist' : 'New setlist'}</DialogTitle>
        </DialogHeader>

        <div className="space-y-4">
          <Input
            placeholder="Setlist name"
            value={name}
            onChange={(e) => setName(e.target.value)}
          />

          <ul className="space-y-2">
            {entries.map((entry, i) => {
              const song = availableSongs.find((s) => s.id === entry.songId)
              return (
                <li key={i} className="flex items-center gap-2 border rounded px-3 py-2">
                  <span className="flex-1 text-sm">{song?.name ?? entry.songId}</span>
                  <Select
                    value={entry.transition}
                    onValueChange={(v) => setTransition(i, v as SetlistTransition)}>
                    <SelectTrigger className="w-32 h-7 text-xs">
                      <SelectValue />
                    </SelectTrigger>
                    <SelectContent>
                      <SelectItem value="stop">Stop</SelectItem>
                      <SelectItem value="continue">Continue</SelectItem>
                    </SelectContent>
                  </Select>
                  <Button size="icon" variant="ghost" className="size-7"
                    onClick={() => moveEntry(i, -1)} disabled={i === 0}>
                    <IconArrowUp className="size-3" />
                  </Button>
                  <Button size="icon" variant="ghost" className="size-7"
                    onClick={() => moveEntry(i, 1)} disabled={i === entries.length - 1}>
                    <IconArrowDown className="size-3" />
                  </Button>
                  <Button size="icon" variant="ghost" className="size-7"
                    onClick={() => removeEntry(i)}>
                    <IconTrash className="size-3" />
                  </Button>
                </li>
              )
            })}
          </ul>

          <Select onValueChange={addSong}>
            <SelectTrigger>
              <SelectValue placeholder="Add a song..." />
            </SelectTrigger>
            <SelectContent>
              {availableSongs.map((s) => (
                <SelectItem key={s.id} value={s.id}>{s.name}</SelectItem>
              ))}
            </SelectContent>
          </Select>
        </div>

        <DialogFooter>
          <Button variant="outline" onClick={onClose}>Cancel</Button>
          <Button onClick={save} disabled={!name.trim()}>Save</Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  )
}
