/**
 * WebSocket Status Indicator
 * Visual indicator showing WebSocket connection state
 */

import React from 'react'
import { cn } from '@/shadcn/lib/utils'

export type ConnectionStatus = 'disconnected' | 'connecting' | 'connected' | 'error'

interface WebSocketStatusProps {
  status: ConnectionStatus
  className?: string
}

const statusConfig = {
  disconnected: {
    color: 'bg-gray-400',
    label: 'Disconnected',
    animate: false
  },
  connecting: {
    color: 'bg-yellow-400',
    label: 'Connecting...',
    animate: true
  },
  connected: {
    color: 'bg-green-500',
    label: 'Connected',
    animate: false
  },
  error: {
    color: 'bg-red-500',
    label: 'Connection Error',
    animate: false
  }
}

export function WebSocketStatus({
  status,
  className
}: Readonly<WebSocketStatusProps>): React.JSX.Element {
  const config = statusConfig[status]

  return (
    <div className={cn('flex items-center gap-2', className)} title={config.label}>
      <div className="relative flex items-center justify-center">
        <div
          className={cn('h-2.5 w-2.5 rounded-full transition-colors duration-300', config.color)}
        />
        {config.animate && (
          <div
            className={cn(
              'absolute h-2.5 w-2.5 rounded-full animate-ping',
              config.color,
              'opacity-75'
            )}
          />
        )}
      </div>
      <span className="text-xs text-muted-foreground">{config.label}</span>
    </div>
  )
}
