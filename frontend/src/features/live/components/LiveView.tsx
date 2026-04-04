import { useSetlist } from '@/shared/contexts/setlist-provider'
import { useProject } from '@/shared/contexts/project-provider'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { LiveHeader } from './LiveHeader'
import { LiveTimeline } from './LiveTimeline'
import { useNavigate } from 'react-router'
import { useEffect } from 'react'
import { IconLoader } from '@tabler/icons-react'
import { LiveTransport } from './LiveTransport'

export function LiveView() {
  const { activeSetlist, currentSong, currentIndex, advance, previous } =
    useSetlist()
  const { playheadPosRef, playheadUpdateRef, playing } = useProject()
  const { send } = useWebSocket()
  const navigate = useNavigate()

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key !== ' ') return
      const tag = (e.target as HTMLElement)?.tagName
      if (tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT') return
      e.preventDefault()
      send({ action: `transport.${playing ? 'pause' : 'play'}` })
    }
    globalThis.addEventListener('keydown', handleKeyDown)
    return () => globalThis.removeEventListener('keydown', handleKeyDown)
  }, [playing, send])

  if (!activeSetlist || !currentSong) {
    return (
      <main className="flex flex-col h-screen items-center justify-center gap-4">
        <IconLoader className="animate-spin size-8" />
        <button
          className="text-sm text-muted-foreground underline"
          onClick={() => navigate('/')}
        >
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
      <LiveTransport />
    </main>
  )
}
