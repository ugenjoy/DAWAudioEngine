import { contextBridge, ipcRenderer } from 'electron'
import { electronAPI } from '@electron-toolkit/preload'

// Custom APIs for renderer
const api = {
  getHealth: () => ipcRenderer.invoke('getHealth'),
  websocket: {
    send: (message: string | object) => ipcRenderer.invoke('websocket:send', message),
    connect: () => ipcRenderer.invoke('websocket:connect'),
    disconnect: () => ipcRenderer.invoke('websocket:disconnect'),
    getStatus: () => ipcRenderer.invoke('websocket:getStatus'),
    onStatus: (callback: (status: string) => void) => {
      const subscription = (_event: any, data: { status: string }): void => callback(data.status)
      ipcRenderer.on('websocket:status', subscription)
      return () => ipcRenderer.removeListener('websocket:status', subscription)
    },
    onMessage: (callback: (data: string) => void) => {
      const subscription = (_event: any, payload: { data: string }): void => callback(payload.data)
      ipcRenderer.on('websocket:message', subscription)
      return () => ipcRenderer.removeListener('websocket:message', subscription)
    },
    onError: (callback: (message: string) => void) => {
      const subscription = (_event: any, payload: { message: string }): void =>
        callback(payload.message)
      ipcRenderer.on('websocket:error', subscription)
      return () => ipcRenderer.removeListener('websocket:error', subscription)
    }
  }
}

// Use `contextBridge` APIs to expose Electron APIs to
// renderer only if context isolation is enabled, otherwise
// just add to the DOM global.
if (process.contextIsolated) {
  try {
    contextBridge.exposeInMainWorld('electron', electronAPI)
    contextBridge.exposeInMainWorld('api', api)
  } catch (error) {
    console.error(error)
  }
} else {
  // @ts-ignore (define in dts)
  globalThis.electron = electronAPI
  // @ts-ignore (define in dts)
  globalThis.api = api
}
