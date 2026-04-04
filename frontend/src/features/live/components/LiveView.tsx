import { useSetlist } from '@/shared/contexts/setlist-provider'
import { useProject } from '@/shared/contexts/project-provider'
import { LiveHeader } from './LiveHeader'
import { LiveTimeline } from './LiveTimeline'
import { Transport } from '@/features/transport/components/Transport'
import { useNavigate } from 'react-router'
import { IconLoader } from '@tabler/icons-react'

export function LiveView() {
  const { activeSetlist, currentSong, currentIndex, advance, previous } = useSetlist()
  const { playheadPosRef, playheadUpdateRef, playing } = useProject()
  const navigate = useNavigate()

  if (!activeSetlist || !currentSong) {
    return (
      <main className="flex flex-col h-screen items-center justify-center gap-4">
        <IconLoader className="animate-spin size-8" />
        <button className="text-sm text-muted-foreground underline" onClick={() => navigate('/')}>
          Back
        </button>
      </main>
    )
  }

  return (
    <main className="flex flex-col h-screen">
      <LiveHeader
        setlist={activeSetlist}
        currentSong={currentSong}
        currentIndex={currentIndex}
        onPrevious={previous}
        onNext={advance}
      />
      <div className="flex-1 overflow-hidden">
        <LiveTimeline
          song={currentSong}
          positionRef={playheadPosRef}
          playheadUpdateRef={playheadUpdateRef}
          playing={playing}
        />
      </div>
      <Transport />
    </main>
  )
}
