import { useEffect } from 'react'
import { useParams } from 'react-router'
import { useProject } from '@/shared/contexts/project-provider'
import Sequencer from '@/features/sequencer/components/Sequencer'
import Navbar from '@/features/navbar/components/Navbar'

export default function EditPage() {
  const { songId } = useParams<{ songId: string }>()
  const { activeSong, loadSong } = useProject()

  useEffect(() => {
    if (songId && activeSong?.id !== songId) {
      loadSong(songId)
    }
  }, [songId])

  return (
    <main className="flex flex-col h-screen w-screen">
      <Navbar />
      <div className="flex-1 overflow-auto">
        <Sequencer />
      </div>
    </main>
  )
}
