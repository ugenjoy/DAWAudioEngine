import { Button } from '@/shared/shadcn/components/button'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { cn } from '@/shared/shadcn/lib/utils'
import { useProject } from '@/shared/contexts/project-provider'
import { useMode } from '@/shared/contexts/mode-provider'
import { useEffect, useState } from 'react'
import { Song } from '@/shared/models/song'
import {
  IconChevronLeft,
  IconChevronRight,
  IconDeviceFloppy,
  IconPencil,
  IconMusic,
} from '@tabler/icons-react'
import { AudioSettingsDialog } from '@/features/audio-settings/components/AudioSettingsDialog'
import { EventsDialog } from '@/features/events/components/EventsDialog'

interface NavbarProps {
  onOpenProjectDialog?: () => void
}

function Navbar({ onOpenProjectDialog }: Readonly<NavbarProps>) {
  const { isConnected, send } = useWebSocket()
  const { activeSong, project, playing, isDirty, saveProject } = useProject()
  const { isLiveMode, setEditMode, setLiveMode } = useMode()
  const [prevSong, setPrevSong] = useState<Song>()
  const [nextSong, setNextSong] = useState<Song>()

  function loadSong(uuid: string) {
    send({
      action: 'project.loadSong',
      uuid,
    })
  }

  useEffect(() => {
    if (project?.songs && activeSong) {
      const activeSongIndex = project.songs.findIndex(
        (s) => s.id === activeSong.id,
      )
      if (activeSongIndex !== undefined) {
        const prevSong = project.songs[activeSongIndex - 1]
        setPrevSong(prevSong)
        const nextSong = project.songs[activeSongIndex + 1]
        setNextSong(nextSong)
      }
    }
  }, [project, activeSong])

  return (
    <div
      className={cn(
        'w-full p-2 min-h-10 items-center grid-cols-[1fr_auto_1fr] grid border-b',
        isLiveMode ? 'border-primary' : 'border-orange-500/50',
      )}
    >
      {/* Mode indicator bar */}
      <div
        className={cn(
          'absolute top-0 left-0 right-0 h-0.5',
          isLiveMode ? 'bg-primary' : 'bg-orange-500',
        )}
      />

      {/* Project name + save */}
      <div className="absolute left-2 flex items-center gap-1">
        <Button
          variant="ghost"
          className="text-md text-foreground hover:text-foreground"
          onClick={onOpenProjectDialog}
        >
          {project?.name}
        </Button>
        {!isLiveMode && (
          <Button
            variant="ghost"
            onClick={saveProject}
            title={isDirty ? 'Save project (unsaved changes)' : 'Save project'}
            className="relative"
          >
            <IconDeviceFloppy size={16} />
            {isDirty && (
              <span className="absolute top-1 right-1 size-1.5 rounded-full bg-orange-400" />
            )}
          </Button>
        )}
      </div>

      <div className="justify-self-end">
        {prevSong && (
          <Button
            variant="ghost"
            onClick={() => loadSong(prevSong.id)}
            disabled={!prevSong}
          >
            {prevSong.name}
            <IconChevronLeft />
          </Button>
        )}
      </div>

      <span className="font-bold mx-4 text-center justify-self-center text-primary">
        {activeSong?.name}
      </span>

      <div className="justify-self-start">
        {nextSong && (
          <Button
            variant="ghost"
            onClick={() => loadSong(nextSong.id)}
            disabled={!nextSong}
          >
            <IconChevronRight />
            {nextSong.name}
          </Button>
        )}
      </div>

      <div className="absolute right-4 text-xs flex items-center gap-3">
        {/* Mode toggle button */}
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
        <div className="flex items-center gap-2">
          <div
            className={cn(
              'rounded-full size-2.5',
              isConnected ? 'bg-chart-4' : 'bg-gray-500',
            )}
          />
          {isConnected ? 'Connected' : 'Disconnected'}
        </div>
      </div>
    </div>
  )
}

export default Navbar
