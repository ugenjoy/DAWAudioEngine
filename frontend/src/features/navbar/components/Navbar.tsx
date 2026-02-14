import { Button } from '@/shared/shadcn/components/button'
import FileDropdownMenu from './FileContextMenu'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { cn } from '@/shared/shadcn/lib/utils'
import { useProject } from '@/shared/contexts/project-provider'
import { useEffect, useState } from 'react'
import { Song } from '@/shared/models/song'

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
    <div className="w-full p-2 items-center flex border-border border-b">
      <FileDropdownMenu />
      <Button variant="ghost">Edit</Button>
      <div className="flex-1" />

      <div>
        {prevSong && (
          <Button variant="ghost" onClick={() => loadSong(prevSong.id)}>
            {prevSong.name}
          </Button>
        )}
        <span className="font-bold">{activeSong?.name}</span>
        {nextSong && (
          <Button variant="ghost" onClick={() => loadSong(nextSong.id)}>
            {nextSong.name}
          </Button>
        )}
      </div>

      <div className="flex-1" />
      <div className="text-sm flex items-baseline gap-2">
        <div
          className={cn(
            'rounded-full size-2.5',
            isConnected ? 'bg-chart-4' : 'bg-gray-500',
          )}
        />
        {isConnected ? 'Connected' : 'Disconnected'}
      </div>
    </div>
  )
}

export default Navbar
