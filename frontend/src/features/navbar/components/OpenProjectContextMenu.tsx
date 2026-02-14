import { useProjects } from '@/shared/contexts/projects-provider'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import {
  DropdownMenuItem,
  DropdownMenuPortal,
  DropdownMenuSub,
  DropdownMenuSubContent,
  DropdownMenuSubTrigger,
} from '@/shared/shadcn/components/dropdown-menu'

function OpenProjectContextMenu() {
  const { isConnected } = useWebSocket()
  const { projects } = useProjects()

  return (
    <DropdownMenuSub>
      <DropdownMenuSubTrigger disabled={!isConnected}>
        Open Project
      </DropdownMenuSubTrigger>
      <DropdownMenuPortal>
        <DropdownMenuSubContent>
          {projects.map((p) => {
            return <DropdownMenuItem key={p.name}>{p.name}</DropdownMenuItem>
          })}
        </DropdownMenuSubContent>
      </DropdownMenuPortal>
    </DropdownMenuSub>
  )
}

export default OpenProjectContextMenu
