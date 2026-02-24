import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { Button } from '@/shared/shadcn/components/button'
import { Input } from '@/shared/shadcn/components/input'
import { IconLoader } from '@tabler/icons-react'
import { useState } from 'react'

function ConnectionPage() {
  const { isConnected, connect, isLoading } = useWebSocket()
  const [ip, setIp] = useState('127.0.0.1')
  const [port, setPort] = useState('8080')

  function handleConnect() {
    connect(`ws://${ip}:${port}/ws`)
  }

  return (
    <main className="flex flex-col h-screen w-screen">
      <div className="m-auto">
        <form
          onSubmit={(e) => {
            e.preventDefault()
            handleConnect()
          }}
          className="flex flex-col gap-6"
        >
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
          <Button type="submit" disabled={isLoading}>
            {isLoading ? (
              <IconLoader className="animate-spin" />
            ) : isConnected ? (
              'Reconnect'
            ) : (
              'Connect'
            )}
          </Button>
        </form>
      </div>
    </main>
  )
}

export default ConnectionPage
