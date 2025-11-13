/**
 * WebSocket Service (Main Process)
 * Manages WebSocket connection in the Electron main process
 */

import { WebSocket } from 'ws'
import { BrowserWindow } from 'electron'
import { rawDataToString } from './utils'

export type ConnectionStatus = 'disconnected' | 'connecting' | 'connected' | 'error'

export class WebSocketService {
  private ws: WebSocket | null = null
  private readonly url: string
  private status: ConnectionStatus = 'disconnected'
  private reconnectAttempts = 0
  private readonly maxReconnectAttempts = 5
  private readonly reconnectInterval = 3000
  private reconnectTimeout: NodeJS.Timeout | null = null
  private isIntentionalDisconnect = false

  constructor(url: string = 'ws://localhost:8080/ws') {
    this.url = url
  }

  /**
   * Connect to WebSocket server
   */
  connect(): void {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      console.log('[WebSocket] Already connected')
      return
    }

    this.isIntentionalDisconnect = false
    this.updateStatus('connecting')

    try {
      console.log('[WebSocket] Connecting to', this.url)
      this.ws = new WebSocket(this.url)

      this.ws.on('open', () => {
        console.log('[WebSocket] Connected')
        this.updateStatus('connected')
        this.reconnectAttempts = 0
      })

      this.ws.on('close', (code, reason) => {
        console.log('[WebSocket] Disconnected', code, reason.toString())
        this.ws = null

        if (this.isIntentionalDisconnect) {
          this.updateStatus('disconnected')
          return
        }

        // Auto-reconnect
        if (this.reconnectAttempts < this.maxReconnectAttempts) {
          this.reconnectAttempts++
          console.log(
            `[WebSocket] Reconnecting... (${this.reconnectAttempts}/${this.maxReconnectAttempts})`
          )
          this.updateStatus('connecting')
          this.reconnectTimeout = setTimeout(() => {
            this.connect()
          }, this.reconnectInterval)
        } else {
          this.updateStatus('disconnected')
        }
      })

      this.ws.on('error', (error) => {
        console.error('[WebSocket] Error:', error)
        this.updateStatus('error')
        this.sendToRenderer('websocket:error', { message: error.message })
      })

      this.ws.on('message', (data) => {
        try {
          const message = rawDataToString(data)
          console.log('[WebSocket] Received:', message)
          this.sendToRenderer('websocket:message', { data: message })
        } catch (error) {
          console.error('[WebSocket] Failed to parse message:', error)
        }
      })
    } catch (error) {
      console.error('[WebSocket] Connection failed:', error)
      this.updateStatus('error')
      const errorMessage = error instanceof Error ? error.message : 'Connection failed'
      this.sendToRenderer('websocket:error', { message: errorMessage })
    }
  }

  /**
   * Disconnect from WebSocket server
   */
  disconnect(): void {
    console.log('[WebSocket] Disconnecting...')
    this.isIntentionalDisconnect = true

    if (this.reconnectTimeout) {
      clearTimeout(this.reconnectTimeout)
      this.reconnectTimeout = null
    }

    if (this.ws) {
      this.ws.close()
      this.ws = null
    }

    this.updateStatus('disconnected')
    this.reconnectAttempts = 0
  }

  /**
   * Send message to WebSocket server
   */
  send(message: string | object): void {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      console.warn('[WebSocket] Cannot send - not connected')
      this.sendToRenderer('websocket:error', { message: 'Not connected' })
      return
    }

    try {
      const data = typeof message === 'string' ? message : JSON.stringify(message)
      console.log('[WebSocket] Sending:', data)
      this.ws.send(data)
    } catch (error) {
      console.error('[WebSocket] Failed to send message:', error)
      const errorMessage = error instanceof Error ? error.message : 'Send failed'
      this.sendToRenderer('websocket:error', { message: errorMessage })
    }
  }

  /**
   * Get current connection status
   */
  getStatus(): ConnectionStatus {
    return this.status
  }

  /**
   * Update status and notify renderer
   */
  private updateStatus(newStatus: ConnectionStatus): void {
    this.status = newStatus
    console.log('[WebSocket] Status changed:', newStatus)
    this.sendToRenderer('websocket:status', { status: newStatus })
  }

  /**
   * Send event to renderer process
   */
  private sendToRenderer(
    channel: string,
    data: { message?: string; status?: string; data?: string }
  ): void {
    const windows = BrowserWindow.getAllWindows()
    for (const window of windows) {
      window.webContents.send(channel, data)
    }
  }
}
