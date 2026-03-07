import { Button } from '@/shared/shadcn/components/button'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { cn } from '@/shared/shadcn/lib/utils'
import { useProject } from '@/shared/contexts/project-provider'
import { useMode } from '@/shared/contexts/mode-provider'
import { useState } from 'react'
import {
  IconChevronRight,
  IconDeviceFloppy,
  IconPencil,
  IconMusic,
} from '@tabler/icons-react'
import { AudioSettingsDialog } from '@/features/audio-settings/components/AudioSettingsDialog'
import { EventsDialog } from '@/features/events/components/EventsDialog'
import { NewSongDialog } from '@/features/songs/components/NewSongDialog'
import { SongManagerDialog } from '@/features/songs/components/SongManagerDialog'

interface NavbarProps {
  onOpenProjectDialog?: () => void
}

function Navbar({ onOpenProjectDialog }: Readonly<NavbarProps>) {
  const { isConnected } = useWebSocket()
  const { activeSong, project, playing, isDirty, saveProject, loadSong } =
    useProject()
  const { isLiveMode, setEditMode, setLiveMode } = useMode()
  const [songManagerOpen, setSongManagerOpen] = useState(false)

  const nextSong = (() => {
    if (!project?.songs || !activeSong) return undefined
    const idx = project.songs.findIndex((s) => s.id === activeSong.id)
    return idx >= 0 ? project.songs[idx + 1] : undefined
  })()

  return (
    <div
      className={cn(
        'w-full p-2 min-h-10 items-center grid-cols-[1fr_auto_1fr] grid border-b',
        isLiveMode ? 'border-primary' : 'border-orange-500/50',
      )}
    >
      <div
        className={cn(
          'absolute top-0 left-0 right-0 h-0.5',
          isLiveMode ? 'bg-primary' : 'bg-orange-500',
        )}
      />

      <div className="absolute left-2 flex items-center gap-1">
        <Button
          variant="ghost"
          className="text-sm text-foreground hover:text-foreground"
          onClick={onOpenProjectDialog}
        >
          {project?.name ?? 'Open project'}
        </Button>
        {!isLiveMode && (
          <>
            <Button
              variant="ghost"
              onClick={saveProject}
              title={
                isDirty ? 'Save project (unsaved changes)' : 'Save project'
              }
              className="relative"
            >
              <IconDeviceFloppy size={16} />
              {isDirty && (
                <span className="absolute top-1 right-1 size-1.5 rounded-full bg-orange-400" />
              )}
            </Button>
            <NewSongDialog />
          </>
        )}
      </div>

      <div />

      <Button
        variant="ghost"
        className="font-bold text-md text-center justify-self-center text-primary hover:text-primary"
        onClick={() => setSongManagerOpen(true)}
        disabled={!project}
      >
        {activeSong?.name ?? 'No song'}
      </Button>

      <div className="justify-self-start">
        {nextSong && (
          <Button variant="ghost" onClick={() => loadSong(nextSong.id)}>
            <IconChevronRight />
            {nextSong.name}
          </Button>
        )}
      </div>

      <SongManagerDialog
        open={songManagerOpen}
        onOpenChange={setSongManagerOpen}
        isLiveMode={isLiveMode}
      />

      <div className="absolute right-4 text-xs flex items-center gap-3">
        {isLiveMode ? (
          <Button
            variant="outline"
            size="sm"
            onClick={setEditMode}
            disabled={playing}
            className="gap-1.5 text-xs border-primary/50 text-primary hover:bg-primary/10"
            title={
              playing
                ? 'Stop playback to enter Edit mode'
                : 'Switch to Edit mode'
            }
          >
            <IconMusic size={14} />
            <span className="font-semibold tracking-wider">LIVE</span>
          </Button>
        ) : (
          <Button
            variant="outline"
            size="sm"
            onClick={setLiveMode}
            className="gap-1.5 text-xs border-orange-500/60 text-orange-400 hover:bg-orange-500/10"
          >
            <IconPencil size={14} />
            <span className="font-semibold tracking-wider">EDIT</span>
          </Button>
        )}

        <EventsDialog />
        <AudioSettingsDialog />
        <button className="flex items-center gap-2 cursor-pointer hover:opacity-80">
          <div
            className={cn(
              'rounded-full size-2.5',
              isConnected ? 'bg-chart-4' : 'bg-gray-500',
            )}
          />
          {isConnected ? 'Connected' : 'Disconnected'}
        </button>
      </div>
    </div>
  )
}

export default Navbar
