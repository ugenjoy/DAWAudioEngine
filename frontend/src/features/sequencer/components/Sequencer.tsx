import { useProject } from '@/shared/contexts/project-provider'
import { Transport } from '../../transport/components/Transport'

function Sequencer() {
  const { project } = useProject()

  return (
    project && (
      <div>
        <Transport />
      </div>
    )
  )
}

export default Sequencer
