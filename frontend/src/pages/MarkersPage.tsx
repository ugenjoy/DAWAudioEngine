import { useEffect, useState } from 'react'
import { useNavigate } from 'react-router'
import { Button } from '@/shared/shadcn/components/button'
import { Input } from '@/shared/shadcn/components/input'
import { IconArrowLeft, IconTrash } from '@tabler/icons-react'
import { useMarkers } from '@/shared/contexts/markers-provider'
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
  AlertDialogTrigger,
} from '@/shared/shadcn/components/alert-dialog'
import { Marker } from '@/shared/models/marker'

function MarkerRow({ marker }: { marker: Marker }) {
  const { updateMarker, removeMarker } = useMarkers()
  const [name, setName] = useState(marker.name)
  const [position, setPosition] = useState(String(marker.position))

  function commitName() {
    const trimmed = name.trim()
    if (trimmed && trimmed !== marker.name) {
      updateMarker(marker.id, { name: trimmed })
    } else {
      setName(marker.name) // revert if empty or unchanged
    }
  }

  function commitPosition() {
    const val = parseFloat(position)
    if (!isNaN(val) && val >= 0 && val !== marker.position) {
      updateMarker(marker.id, { position: val })
    } else {
      setPosition(String(marker.position)) // revert
    }
  }

  return (
    <div className="flex items-center gap-3 py-2 border-b">
      <Input
        value={name}
        onChange={(e) => setName(e.target.value)}
        onBlur={commitName}
        onKeyDown={(e) => { if (e.key === 'Enter') e.currentTarget.blur() }}
        className="flex-1 h-7 text-sm"
      />
      <div className="flex items-center gap-1">
        <Input
          type="number"
          min={0}
          step={0.01}
          value={position}
          onChange={(e) => setPosition(e.target.value)}
          onBlur={commitPosition}
          onKeyDown={(e) => { if (e.key === 'Enter') e.currentTarget.blur() }}
          className="w-24 h-7 text-sm"
        />
        <span className="text-xs text-muted-foreground">s</span>
      </div>
      <AlertDialog>
        <AlertDialogTrigger asChild>
          <Button variant="ghost" size="icon-sm" title="Delete marker">
            <IconTrash size={14} />
          </Button>
        </AlertDialogTrigger>
        <AlertDialogContent>
          <AlertDialogHeader>
            <AlertDialogTitle>Delete marker?</AlertDialogTitle>
            <AlertDialogDescription>
              Position triggers referencing this marker will no longer fire.
            </AlertDialogDescription>
          </AlertDialogHeader>
          <AlertDialogFooter>
            <AlertDialogCancel>Cancel</AlertDialogCancel>
            <AlertDialogAction onClick={() => removeMarker(marker.id)}>
              Delete
            </AlertDialogAction>
          </AlertDialogFooter>
        </AlertDialogContent>
      </AlertDialog>
    </div>
  )
}

export default function MarkersPage() {
  const navigate = useNavigate()
  const { markers, fetchMarkers } = useMarkers()

  useEffect(() => { fetchMarkers() }, [fetchMarkers])

  const sorted = [...markers].sort((a, b) => a.position - b.position)

  return (
    <main className="flex flex-col h-screen w-screen">
      <div className="flex items-center gap-3 px-4 py-3 border-b shrink-0">
        <Button variant="ghost" size="sm" onClick={() => navigate(-1)} className="gap-1.5">
          <IconArrowLeft size={14} />
          Back
        </Button>
        <h1 className="text-sm font-semibold">Markers</h1>
      </div>

      <div className="flex-1 overflow-y-auto p-4 max-w-2xl mx-auto w-full">
        {sorted.length === 0 ? (
          <p className="text-sm text-muted-foreground text-center mt-8">
            No markers yet. Right-click on the timeline to add one.
          </p>
        ) : (
          <div className="flex flex-col">
            <div className="flex items-center gap-3 pb-1 text-xs text-muted-foreground font-medium">
              <span className="flex-1">Name</span>
              <span className="w-24">Position</span>
              <span className="w-7" />
            </div>
            {sorted.map((m) => (
              <MarkerRow key={m.id} marker={m} />
            ))}
          </div>
        )}
      </div>
    </main>
  )
}
