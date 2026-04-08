import { useNavigate } from 'react-router'
import { Button } from '@/shared/shadcn/components/button'
import { IconArrowLeft } from '@tabler/icons-react'
import { EventsPanel } from '@/features/events/components/EventsDialog'

export default function EventsPage() {
  const navigate = useNavigate()

  return (
    <main className="flex flex-col h-screen w-screen">
      <div className="flex items-center gap-3 px-4 py-3 border-b shrink-0">
        <Button variant="ghost" size="sm" onClick={() => navigate(-1)} className="gap-1.5">
          <IconArrowLeft size={14} />
          Back
        </Button>
        <h1 className="text-sm font-semibold">Events</h1>
      </div>
      <EventsPanel />
    </main>
  )
}
