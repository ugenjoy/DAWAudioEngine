import { useState, useRef, useEffect } from 'react'
import { useProject } from '@/shared/contexts/project-provider'
import { Button } from '@/shared/shadcn/components/button'
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
} from '@/shared/shadcn/components/dialog'
import { Input } from '@/shared/shadcn/components/input'
import { cn } from '@/shared/shadcn/lib/utils'
import {
  IconGripVertical,
  IconPencil,
  IconCheck,
  IconX,
} from '@tabler/icons-react'
import { Song } from '@/shared/models/song'

interface SongManagerDialogProps {
  open: boolean
  onOpenChange: (open: boolean) => void
  isLiveMode: boolean
}

export function SongManagerDialog({
  open,
  onOpenChange,
  isLiveMode,
}: Readonly<SongManagerDialogProps>) {
  const { project, activeSong, loadSong, renameSong, reorderSong } =
    useProject()
  const [editingId, setEditingId] = useState<string | null>(null)
  const [editName, setEditName] = useState('')
  const [dragIndex, setDragIndex] = useState<number | null>(null)
  const [overIndex, setOverIndex] = useState<number | null>(null)
  const inputRef = useRef<HTMLInputElement>(null)

  const songs = project?.songs ?? []

  useEffect(() => {
    if (editingId && inputRef.current) {
      inputRef.current.focus()
      inputRef.current.select()
    }
  }, [editingId])

  function startRename(song: Song) {
    setEditingId(song.id)
    setEditName(song.name)
  }

  function confirmRename() {
    if (!editingId) return
    const trimmed = editName.trim()
    if (trimmed) {
      renameSong(editingId, trimmed)
    }
    setEditingId(null)
  }

  function cancelRename() {
    setEditingId(null)
  }

  function handleSongClick(song: Song) {
    if (song.id === activeSong?.id) return
    loadSong(song.id)
    onOpenChange(false)
  }

  function handleDragStart(index: number) {
    setDragIndex(index)
  }

  function handleDragOver(e: React.DragEvent, index: number) {
    e.preventDefault()
    setOverIndex(index)
  }

  function handleDrop(index: number) {
    if (dragIndex === null || dragIndex === index) {
      setDragIndex(null)
      setOverIndex(null)
      return
    }
    const song = songs[dragIndex]
    reorderSong(song.id, index)
    setDragIndex(null)
    setOverIndex(null)
  }

  function handleDragEnd() {
    setDragIndex(null)
    setOverIndex(null)
  }

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent className="max-w-sm">
        <DialogHeader>
          <DialogTitle>Songs</DialogTitle>
        </DialogHeader>
        <div className="flex flex-col gap-1 max-h-80 overflow-y-auto">
          {songs.map((song, index) => (
            <div
              key={song.id}
              draggable={!isLiveMode && editingId !== song.id}
              onDragStart={!isLiveMode ? () => handleDragStart(index) : undefined}
              onDragOver={!isLiveMode ? (e) => handleDragOver(e, index) : undefined}
              onDrop={!isLiveMode ? () => handleDrop(index) : undefined}
              onDragEnd={!isLiveMode ? handleDragEnd : undefined}
              className={cn(
                'flex items-center gap-2 rounded px-2 py-1.5 group',
                'hover:bg-muted/50 cursor-pointer',
                song.id === activeSong?.id && 'bg-primary/10 text-primary',
                dragIndex === index && 'opacity-40',
                overIndex === index &&
                  dragIndex !== null &&
                  dragIndex !== index &&
                  'border-t-2 border-primary',
              )}
            >
              {!isLiveMode && (
                <IconGripVertical
                  size={14}
                  className="shrink-0 text-muted-foreground cursor-grab"
                />
              )}

              {!isLiveMode && editingId === song.id ? (
                <form
                  className="flex items-center gap-1 flex-1"
                  onSubmit={(e) => {
                    e.preventDefault()
                    confirmRename()
                  }}
                >
                  <Input
                    ref={inputRef}
                    value={editName}
                    onChange={(e) => setEditName(e.target.value)}
                    className="h-6 text-sm"
                    onKeyDown={(e) => {
                      if (e.key === 'Escape') cancelRename()
                    }}
                    onBlur={confirmRename}
                  />
                  <Button
                    type="submit"
                    variant="ghost"
                    size="icon-sm"
                    className="shrink-0"
                  >
                    <IconCheck size={14} />
                  </Button>
                  <Button
                    type="button"
                    variant="ghost"
                    size="icon-sm"
                    className="shrink-0"
                    onMouseDown={(e) => {
                      e.preventDefault()
                      cancelRename()
                    }}
                  >
                    <IconX size={14} />
                  </Button>
                </form>
              ) : (
                <>
                  <span
                    className="flex-1 text-sm truncate"
                    onClick={() => handleSongClick(song)}
                  >
                    {song.name}
                  </span>
                  {!isLiveMode && (
                    <Button
                      variant="ghost"
                      size="icon-sm"
                      className="shrink-0 opacity-0 group-hover:opacity-100"
                      onClick={(e) => {
                        e.stopPropagation()
                        startRename(song)
                      }}
                    >
                      <IconPencil size={14} />
                    </Button>
                  )}
                </>
              )}
            </div>
          ))}
          {songs.length === 0 && (
            <p className="text-sm text-muted-foreground text-center py-4">
              No songs in this project
            </p>
          )}
        </div>
      </DialogContent>
    </Dialog>
  )
}
