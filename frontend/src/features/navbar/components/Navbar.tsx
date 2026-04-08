import { Button } from '@/shared/shadcn/components/button'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { cn } from '@/shared/shadcn/lib/utils'
import { useProject } from '@/shared/contexts/project-provider'
import { useMode } from '@/shared/contexts/mode-provider'
import { useSetlist } from '@/shared/contexts/setlist-provider'
import { useState } from 'react'
import { useNavigate, useLocation } from 'react-router'
import { IconDeviceFloppy, IconHome, IconBroadcast, IconBolt } from '@tabler/icons-react'
import { AudioSettingsDialog } from '@/features/audio-settings/components/AudioSettingsDialog'
import { NewSongDialog } from '@/features/songs/components/NewSongDialog'
import { SongManagerDialog } from '@/features/songs/components/SongManagerDialog'
import { useNavigateGuarded } from '@/shared/hooks/useNavigateGuarded'
import { ConfirmStopDialog } from '@/shared/components/ConfirmStopDialog'

function Navbar() {
  const { isConnected } = useWebSocket()
  const { activeSong, project, isDirty, saveProject } = useProject()
  const { isLiveMode } = useMode()
  const { loadSingle } = useSetlist()
  const navigate = useNavigate()
  const location = useLocation()
  const { guardedNavigate, confirmStop, cancelStop, showConfirm } =
    useNavigateGuarded()
  const [songManagerOpen, setSongManagerOpen] = useState(false)

  const isEditRoute = location.pathname.startsWith('/edit')

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
          size="sm"
          onClick={() => guardedNavigate('/')}
          className="gap-1.5 text-xs"
          title="Go to home"
        >
          <IconHome size={14} />
        </Button>
        <h2 className="text-sm text-foreground hover:text-foreground">
          {project?.name ?? 'Open project'}
        </h2>
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

      {/* <div className="justify-self-start">
        {nextSong && (
          <Button variant="ghost" onClick={() => loadSong(nextSong.id)}>
            <IconChevronRight />
            {nextSong.name}
          </Button>
        )}
      </div> */}

      <div className="absolute right-4 text-xs flex items-center gap-3">
        {isEditRoute && (
          <Button
            variant="outline"
            size="sm"
            onClick={() => { if (activeSong) loadSingle(activeSong.id); navigate('/live') }}
            className="gap-1.5 text-xs border-primary/50 text-primary hover:bg-primary/10"
            title="Go to Live view"
          >
            <IconBroadcast size={14} />
            <span className="font-semibold tracking-wider">GO LIVE</span>
          </Button>
        )}

        <SongManagerDialog
          open={songManagerOpen}
          onOpenChange={setSongManagerOpen}
          isLiveMode={isLiveMode}
        />
        <Button
          variant="ghost"
          size="icon-sm"
          onClick={() => navigate('/events')}
          disabled={isLiveMode}
          title="Events"
        >
          <IconBolt size={16} />
        </Button>
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

      <ConfirmStopDialog
        open={showConfirm}
        onConfirm={confirmStop}
        onCancel={cancelStop}
      />
    </div>
  )
}

export default Navbar
