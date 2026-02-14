import { Button } from '@/shared/shadcn/components/button'

import OpenProjectContextMenu from './OpenProjectContextMenu'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuGroup,
  DropdownMenuItem,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
} from '@/shared/shadcn/components/dropdown-menu'

function FileDropdownMenu() {
  const { isConnected } = useWebSocket()

  return (
    <DropdownMenu>
      <DropdownMenuTrigger asChild>
        <Button variant="ghost">File</Button>
      </DropdownMenuTrigger>
      <DropdownMenuContent>
        <DropdownMenuGroup>
          <DropdownMenuItem disabled={!isConnected}>
            New Project
          </DropdownMenuItem>
          <OpenProjectContextMenu />
        </DropdownMenuGroup>
        <DropdownMenuGroup>
          <DropdownMenuSeparator />
          <DropdownMenuItem>About</DropdownMenuItem>
        </DropdownMenuGroup>
      </DropdownMenuContent>
    </DropdownMenu>
  )
}

export default FileDropdownMenu
