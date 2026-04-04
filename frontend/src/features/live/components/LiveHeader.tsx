import { Button } from '@/shared/shadcn/components/button'
import { IconChevronLeft, IconChevronRight, IconArrowLeft, IconEdit } from '@tabler/icons-react'
import { Setlist } from '@/shared/models/setlist'
import { Song } from '@/shared/models/song'
import { ConfirmStopDialog } from '@/shared/components/ConfirmStopDialog'
import { useNavigateGuarded } from '@/shared/hooks/useNavigateGuarded'

interface Props {
  setlist: Setlist
  currentSong: Song
  currentIndex: number
  onPrevious: () => void
  onNext: () => void
}

export function LiveHeader({ setlist, currentSong, currentIndex, onPrevious, onNext }: Props) {
  const { guardedNavigate, confirmStop, cancelStop, showConfirm } = useNavigateGuarded()
  const total = setlist.entries.length
  const nextEntry = setlist.entries[currentIndex + 1]

  return (
    <header className="flex items-center justify-between px-4 py-2 border-b bg-background">
      <div className="flex items-center gap-4">
        <Button variant="ghost" size="sm" onClick={() => guardedNavigate('/')}>
          <IconArrowLeft className="size-4 mr-1" /> Home
        </Button>
        <Button variant="ghost" size="sm" onClick={() => guardedNavigate(`/edit/${currentSong.id}`)}>
          <IconEdit className="size-4 mr-1" /> Edit
        </Button>
      </div>

      <div className="flex flex-col items-center">
        <span className="font-semibold">{setlist.name}</span>
        <span className="text-sm text-muted-foreground">{currentSong.name}</span>
        <span className="text-xs text-muted-foreground">
          {currentIndex + 1}/{total} · {currentSong.tempo} BPM
        </span>
      </div>

      <div className="flex items-center gap-2">
        <Button size="sm" variant="outline" disabled={currentIndex === 0} onClick={onPrevious}>
          <IconChevronLeft className="size-4" />
        </Button>
        <div className="text-xs text-muted-foreground text-right min-w-24">
          {nextEntry
            ? <span>Next: {nextEntry.transition === 'continue' ? '→' : '■'}</span>
            : <span>End of setlist</span>}
        </div>
        <Button size="sm" variant="outline" disabled={currentIndex >= total - 1} onClick={onNext}>
          <IconChevronRight className="size-4" />
        </Button>
      </div>

      <ConfirmStopDialog open={showConfirm} onConfirm={confirmStop} onCancel={cancelStop} />
    </header>
  )
}
