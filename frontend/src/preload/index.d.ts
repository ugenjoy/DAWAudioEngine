import { ElectronAPI } from '@electron-toolkit/preload'

interface API {
  getHealth: () => Promise<string>
  websocket: {
    send: (message: string | object) => Promise<void>
    connect: () => Promise<void>
    disconnect: () => Promise<void>
    getStatus: () => Promise<string>
    onStatus: (callback: (status: string) => void) => () => void
    onMessage: (callback: (data: string) => void) => () => void
    onError: (callback: (message: string) => void) => () => void
  }
}

declare global {
  interface Window {
    electron: ElectronAPI
    api: API
  }
}
