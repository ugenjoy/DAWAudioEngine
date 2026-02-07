import { useProject } from '@/shared/contexts/project-provider'
import Track from './Track'
import Toolbar from '../toolbar/components/Toolbar'

function Sequencer() {
  const { project } = useProject()

  return (
    project && (
      <div>
        <Toolbar />
        {project.tracks.map((track) => (
          <Track key={track.id} trackId={track.id} />
        ))}
      </div>
    )
  )
}

export default Sequencer
