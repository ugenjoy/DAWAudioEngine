import { useEffect, useRef, useState } from 'react'
import { useNavigate } from 'react-router'
import {
  IconPlayerPlay,
  IconEdit,
  IconPlus,
  IconTrash,
  IconX,
  IconArrowUp,
  IconArrowDown,
  IconGripVertical,
} from '@tabler/icons-react'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { useProject } from '@/shared/contexts/project-provider'
import { useSetlist } from '@/shared/contexts/setlist-provider'
import ProjectsDialog from '@/features/projects/components/ProjectsDialog'
import {
  Setlist,
  SetlistEntry,
  SetlistTransition,
} from '@/shared/models/setlist'
import { Button } from '@/shared/shadcn/components/button'
import { Input } from '@/shared/shadcn/components/input'
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from '@/shared/shadcn/components/select'

export default function ProjectHomePage() {
  const { isConnected } = useWebSocket()
  const { project, createSong } = useProject()
  const {
    setlists,
    loadSetlist,
    loadSingle,
    createSetlist,
    updateSetlist,
    deleteSetlist,
  } = useSetlist()
  const navigate = useNavigate()

  const [projectsDialogOpen, setProjectsDialogOpen] = useState(false)
  const [editingSetlist, setEditingSetlist] = useState<Setlist | null | 'new'>(
    null,
  )

  // Setlist editor state
  const [editorName, setEditorName] = useState('')
  const [editorEntries, setEditorEntries] = useState<SetlistEntry[]>([])
  const dragIndex = useRef<number | null>(null)

  // New-song inline editor state
  const [creatingSong, setCreatingSong] = useState(false)
  const [newSongName, setNewSongName] = useState('')

  function submitNewSong() {
    const trimmed = newSongName.trim()
    if (!trimmed) return
    createSong(trimmed)
    setNewSongName('')
    setCreatingSong(false)
  }

  function cancelNewSong() {
    setNewSongName('')
    setCreatingSong(false)
  }

  useEffect(() => {
    setProjectsDialogOpen(isConnected && project === null)
  }, [project, isConnected])

  function openEditor(sl: Setlist | 'new') {
    setEditingSetlist(sl)
    if (sl === 'new') {
      setEditorName('')
      setEditorEntries([])
    } else {
      setEditorName(sl.name)
      setEditorEntries([...sl.entries])
    }
  }

  function closeEditor() {
    setEditingSetlist(null)
  }

  function saveEditor() {
    if (!editorName.trim()) return
    if (editingSetlist === 'new') {
      createSetlist(editorName, editorEntries, [])
    } else if (editingSetlist) {
      updateSetlist(editingSetlist.id, editorName, editorEntries, undefined)
    }
    closeEditor()
  }

  function addSong(songId: string) {
    setEditorEntries([...editorEntries, { songId, transition: 'stop' }])
  }

  function removeEntry(index: number) {
    setEditorEntries(editorEntries.filter((_, i) => i !== index))
  }

  function moveEntry(index: number, direction: -1 | 1) {
    const next = [...editorEntries]
    const swap = index + direction
    if (swap < 0 || swap >= next.length) return
    ;[next[index], next[swap]] = [next[swap], next[index]]
    setEditorEntries(next)
  }

  function setTransition(index: number, transition: SetlistTransition) {
    const next = [...editorEntries]
    next[index] = { ...next[index], transition }
    setEditorEntries(next)
  }

  // Drag-and-drop reorder
  function onDragStart(index: number) {
    dragIndex.current = index
  }

  function onDrop(targetIndex: number) {
    if (dragIndex.current === null || dragIndex.current === targetIndex) return
    const next = [...editorEntries]
    const [moved] = next.splice(dragIndex.current, 1)
    next.splice(targetIndex, 0, moved)
    setEditorEntries(next)
    dragIndex.current = null
  }

  // Drop from songs list onto setlist
  function onDropSong(e: React.DragEvent) {
    e.preventDefault()
    const songId = e.dataTransfer.getData('songId')
    if (songId) addSong(songId)
  }

  if (!project) {
    return (
      <ProjectsDialog
        open={projectsDialogOpen}
        setOpen={setProjectsDialogOpen}
      />
    )
  }

  const availableSongs = project.songs ?? []

  return (
    <main className="flex flex-col h-screen">
      <header className="flex items-center justify-between px-4 py-2 border-b">
        <h1 className="font-semibold">{project.name}</h1>
        <Button
          variant="outline"
          size="sm"
          onClick={() => setProjectsDialogOpen(true)}
        >
          Change project
        </Button>
      </header>

      <div className="flex flex-1 min-h-0">
        {/* Songs panel */}
        <section className="flex-1 flex flex-col border-r">
          <div className="flex items-center justify-between px-4 pt-4 pb-2">
            <h2 className="text-lg font-medium">Songs</h2>
            <Button
              size="sm"
              onClick={() => setCreatingSong(true)}
              disabled={creatingSong}
            >
              <IconPlus className="size-4 mr-1" /> New song
            </Button>
          </div>
          {creatingSong && (
            <div className="flex gap-2 px-4 pb-2">
              <Input
                autoFocus
                placeholder="Song name"
                value={newSongName}
                onChange={(e) => setNewSongName(e.target.value)}
                onKeyDown={(e) => {
                  if (e.key === 'Enter') submitNewSong()
                  else if (e.key === 'Escape') cancelNewSong()
                }}
              />
              <Button
                onClick={submitNewSong}
                disabled={!newSongName.trim()}
              >
                Create
              </Button>
              <Button variant="outline" onClick={cancelNewSong}>
                Cancel
              </Button>
            </div>
          )}
          <ul className="flex-1 overflow-y-auto space-y-2 px-4 pb-4">
            {availableSongs.map((song) => (
              <li
                key={song.id}
                draggable={editingSetlist !== null}
                onDragStart={(e) => {
                  e.dataTransfer.setData('songId', song.id)
                  e.dataTransfer.effectAllowed = 'copy'
                }}
                className="flex items-center justify-between rounded-md border px-4 py-2 cursor-default"
              >
                {editingSetlist !== null && (
                  <IconGripVertical className="size-4 text-muted-foreground mr-2 shrink-0 cursor-grab" />
                )}
                <span className="flex-1 min-w-0 truncate">
                  {song.name}
                  <span className="text-muted-foreground text-sm ml-2">
                    {song.tempo} BPM
                  </span>
                </span>
                <div className="flex gap-2 shrink-0">
                  <Button
                    size="sm"
                    variant="outline"
                    onClick={() => {
                      loadSingle(song.id)
                      navigate('/live')
                    }}
                  >
                    <IconPlayerPlay className="size-4" />
                  </Button>
                  <Button
                    size="sm"
                    variant="outline"
                    onClick={() => navigate(`/edit/${song.id}`)}
                  >
                    <IconEdit className="size-4" />
                  </Button>
                </div>
              </li>
            ))}
          </ul>
        </section>

        {/* Setlists panel or inline editor */}
        {editingSetlist !== null ? (
          <section
            className="flex-1 flex flex-col"
            onDragOver={(e) => e.preventDefault()}
            onDrop={onDropSong}
          >
            <div className="flex items-center justify-between px-4 pt-4 pb-2 border-b">
              <h2 className="text-lg font-medium">
                {editingSetlist === 'new' ? 'New setlist' : 'Edit setlist'}
              </h2>
              <Button size="icon" variant="ghost" onClick={closeEditor}>
                <IconX className="size-4" />
              </Button>
            </div>

            <div className="px-4 pt-3 pb-2">
              <Input
                placeholder="Setlist name"
                value={editorName}
                onChange={(e) => setEditorName(e.target.value)}
              />
            </div>

            <ul className="flex-1 overflow-y-auto space-y-2 px-4 pb-2">
              {editorEntries.map((entry, i) => {
                const song = availableSongs.find((s) => s.id === entry.songId)
                return (
                  <li
                    key={i}
                    draggable
                    onDragStart={() => onDragStart(i)}
                    onDragOver={(e) => e.preventDefault()}
                    onDrop={(e) => {
                      e.stopPropagation()
                      onDrop(i)
                    }}
                    className="flex items-center gap-2 border rounded px-3 py-2 cursor-grab bg-background"
                  >
                    <IconGripVertical className="size-4 text-muted-foreground shrink-0" />
                    <span className="flex-1 text-sm truncate">
                      {song?.name ?? entry.songId}
                    </span>
                    <Select
                      value={entry.transition}
                      onValueChange={(v) =>
                        setTransition(i, v as SetlistTransition)
                      }
                    >
                      <SelectTrigger className="w-28 h-7 text-xs shrink-0">
                        <SelectValue />
                      </SelectTrigger>
                      <SelectContent>
                        <SelectItem value="stop">Stop</SelectItem>
                        <SelectItem value="pause">Pause</SelectItem>
                        <SelectItem value="continue">Continue</SelectItem>
                      </SelectContent>
                    </Select>
                    <Button
                      size="icon"
                      variant="ghost"
                      className="size-7 shrink-0"
                      onClick={() => moveEntry(i, -1)}
                      disabled={i === 0}
                    >
                      <IconArrowUp className="size-3" />
                    </Button>
                    <Button
                      size="icon"
                      variant="ghost"
                      className="size-7 shrink-0"
                      onClick={() => moveEntry(i, 1)}
                      disabled={i === editorEntries.length - 1}
                    >
                      <IconArrowDown className="size-3" />
                    </Button>
                    <Button
                      size="icon"
                      variant="ghost"
                      className="size-7 shrink-0"
                      onClick={() => removeEntry(i)}
                    >
                      <IconTrash className="size-3" />
                    </Button>
                  </li>
                )
              })}
              {editorEntries.length === 0 && (
                <li className="flex items-center justify-center h-20 border-2 border-dashed rounded text-sm text-muted-foreground">
                  Drag songs here or use the selector below
                </li>
              )}
            </ul>

            <div className="px-4 pb-3">
              <Select onValueChange={addSong}>
                <SelectTrigger>
                  <SelectValue placeholder="Add a song..." />
                </SelectTrigger>
                <SelectContent>
                  {availableSongs.map((s) => (
                    <SelectItem key={s.id} value={s.id}>
                      {s.name}
                    </SelectItem>
                  ))}
                </SelectContent>
              </Select>
            </div>

            <div className="flex gap-2 px-4 pb-4">
              <Button
                variant="outline"
                className="flex-1"
                onClick={closeEditor}
              >
                Cancel
              </Button>
              <Button
                className="flex-1"
                onClick={saveEditor}
                disabled={!editorName.trim()}
              >
                Save
              </Button>
            </div>
          </section>
        ) : (
          <section className="flex-1 flex flex-col">
            <div className="flex items-center justify-between px-4 pt-4 pb-2">
              <h2 className="text-lg font-medium">Setlists</h2>
              <Button size="sm" onClick={() => openEditor('new')}>
                <IconPlus className="size-4 mr-1" /> New setlist
              </Button>
            </div>
            <ul className="flex-1 overflow-y-auto space-y-2 px-4 pb-4">
              {setlists.map((sl) => (
                <li
                  key={sl.id}
                  className="flex items-center justify-between rounded-md border px-4 py-2"
                >
                  <span className="min-w-0 truncate">
                    {sl.name}
                    <span className="text-muted-foreground text-sm ml-2">
                      {sl.entries.length} songs
                    </span>
                  </span>
                  <div className="flex gap-2 shrink-0">
                    <Button
                      size="sm"
                      variant="outline"
                      onClick={() => {
                        loadSetlist(sl.id)
                        navigate('/live')
                      }}
                    >
                      <IconPlayerPlay className="size-4" />
                    </Button>
                    <Button
                      size="sm"
                      variant="outline"
                      onClick={() => openEditor(sl)}
                    >
                      <IconEdit className="size-4" />
                    </Button>
                    <Button
                      size="sm"
                      variant="destructive"
                      onClick={() => deleteSetlist(sl.id)}
                    >
                      <IconTrash className="size-4" />
                    </Button>
                  </div>
                </li>
              ))}
            </ul>
          </section>
        )}
      </div>

      <ProjectsDialog
        open={projectsDialogOpen}
        setOpen={setProjectsDialogOpen}
      />
    </main>
  )
}
