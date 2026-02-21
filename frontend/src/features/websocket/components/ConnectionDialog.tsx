import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { Button } from '@/shared/shadcn/components/button'
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
} from '@/shared/shadcn/components/dialog'
import { Input } from '@/shared/shadcn/components/input'
import { IconLoader } from '@tabler/icons-react'
import { useState } from 'react'

interface ConnectionDialogProps {
  open: boolean
  setOpen: (value: boolean) => void
}

function ConnectionDialog({ open, setOpen }: Readonly<ConnectionDialogProps>) {
  const { connect, isLoading } = useWebSocket()

  const [ip, setIp] = useState('127.0.0.1')
  const [port, setPort] = useState('8080')

  function handleConnect() {
    connect(`ws://${ip}:${port}/ws`)
  }

  return (
    <Dialog open={open} onOpenChange={setOpen}>
      <DialogContent className="flex flex-col gap-6 m-auto">
        <DialogHeader>
          <DialogTitle>Connect to server</DialogTitle>
        </DialogHeader>
        <Input
          placeholder="127.0.0.1"
          value={ip}
          onChange={(e) => setIp(e.target.value)}
        />
        <Input
          placeholder="8080"
          value={port}
          onChange={(e) => setPort(e.target.value)}
        />
        <Button onClick={handleConnect}>
          {isLoading ? <IconLoader className="animate-spin" /> : 'Connect'}
        </Button>
      </DialogContent>
    </Dialog>
  )
}

export default ConnectionDialog
