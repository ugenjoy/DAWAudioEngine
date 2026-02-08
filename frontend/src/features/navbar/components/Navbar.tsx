import { Button } from '@/shared/shadcn/components/button'
import FileDropdownMenu from './FileContextMenu'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { cn } from '@/shared/shadcn/lib/utils'

function Navbar() {
  const { isConnected } = useWebSocket()

  return (
    <div className="w-full p-2 items-center flex border-border border-b">
      <FileDropdownMenu />
      <Button variant="ghost">Edit</Button>
      <div className="flex-1" />
      <div className="text-sm flex items-baseline gap-2">
        <div
          className={cn(
            'rounded-full size-2.5',
            isConnected ? 'bg-chart-4' : 'bg-gray-500',
          )}
        />
        {isConnected ? 'Connected' : 'Disconnected'}
      </div>
    </div>
  )
}

export default Navbar
