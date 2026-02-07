import Navbar from '@/features/navbar/Navbar'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import React, { useEffect, useState } from 'react'
import { Outlet } from 'react-router'
import { Input } from '../shadcn/components/input'
import { Button } from '../shadcn/components/button'

function MainLayout(): React.JSX.Element {
  const { connect, ws } = useWebSocket()
  const [ip, setIp] = useState('127.0.0.1')
  const [port, setPort] = useState('8080')

  function handleConnect() {
    connect(`ws://${ip}:${port}/ws`)
  }

  useEffect(() => {
    if (ws) {
      console.log('ws : ', ws)
    }
  }, [ws])

  return (
    <main className="flex flex-col h-screen">
      <Navbar />
      {ws ? (
        <div className="flex-1 overflow-auto">
          <Outlet />
        </div>
      ) : (
        <div className="flex flex-col gap-6 m-auto">
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
          <Button onClick={handleConnect}>Connect</Button>
        </div>
      )}
    </main>
  )
}

export default MainLayout
