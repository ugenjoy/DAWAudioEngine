/**
 * API Types
 * Type definitions for HTTP API responses
 */

export interface HealthResponse {
  status: 'ok' | 'error'
  message?: string
}

export interface ApiError {
  message: string
  code?: string
}

/**
 * WebSocket Message Types
 */
export interface WebSocketMessage {
  type: string
  payload: any
}

export interface WebSocketEchoMessage extends WebSocketMessage {
  type: 'echo'
  payload: {
    message: string
  }
}
