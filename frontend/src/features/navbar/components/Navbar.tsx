import { Button } from '@/shared/shadcn/components/button'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { cn } from '@/shared/shadcn/lib/utils'
import { useProject } from '@/shared/contexts/project-provider'
import { useEffect, useState } from 'react'
import { Song } from '@/shared/models/song'
import { IconChevronLeft, IconChevronRight } from '@tabler/icons-react'
import { AudioSettingsDialog } from '@/features/audio-settings/components/AudioSettingsDialog'

function Navbar() {
  const { isConnected, send } = useWebSocket()
  const { activeSong, project } = useProject()
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
    <div className="w-full p-2 items-center grid-cols-[1fr_auto_1fr] grid border-border border-b">
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
