import { ElectronAPI } from '@electron-toolkit/preload'

interface API {
  getHealth: () => Promise<string>
}

declare global {
  interface Window {
    electron: ElectronAPI
    api: API
  }
}
