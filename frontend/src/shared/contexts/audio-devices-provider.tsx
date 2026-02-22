import { createContext, useCallback, useContext, useEffect, useState } from 'react'
import { useWebSocket } from './websocket-provider'
import { AudioDeviceType, AudioDeviceInfo } from '../models/audio-input'

type AudioDevicesProviderState = {
  deviceTypes: AudioDeviceType[]
  currentDevice: AudioDeviceInfo | null
  fetchDevices: () => void
  setAudioDevice: (
    deviceType: string,
    outputDevice: string,
    inputDevice: string,
    sampleRate?: number,
    bufferSize?: number,
  ) => void
}

const initialState: AudioDevicesProviderState = {
  deviceTypes: [],
  currentDevice: null,
  fetchDevices: () => null,
  setAudioDevice: () => null,
}

const AudioDevicesContext = createContext<AudioDevicesProviderState>(initialState)

export function AudioDevicesProvider({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  const { ws, isConnected, send } = useWebSocket()
  const [deviceTypes, setDeviceTypes] = useState<AudioDeviceType[]>([])
  const [currentDevice, setCurrentDevice] = useState<AudioDeviceInfo | null>(
    null,
  )

  const fetchDevices = useCallback(() => {
    if (ws && isConnected) {
      send({ action: 'audio.listDevices' })
    }
  }, [ws, isConnected, send])

  const setAudioDevice = useCallback(
    (deviceType: string, outputDevice: string, inputDevice: string, sampleRate?: number, bufferSize?: number) => {
      send({
        action: 'audio.setDevice',
        deviceType,
        outputDevice,
        inputDevice,
        ...(sampleRate !== undefined && { sampleRate }),
        ...(bufferSize !== undefined && { bufferSize }),
      })
    },
    [send],
  )

  useEffect(() => {
    if (ws && isConnected) {
      ws.addEventListener('message', onMessage)
      send({ action: 'audio.listDevices' })
      return () => ws.removeEventListener('message', onMessage)
    }
  }, [ws, isConnected])

  function onMessage(ev: MessageEvent<unknown>) {
    if (typeof ev.data !== 'string') return
    const data = JSON.parse(ev.data)
    switch (data.event) {
      case 'audio.devicesList': {
        if (data.deviceTypes !== undefined) setDeviceTypes(data.deviceTypes)
        if (data.current !== undefined) setCurrentDevice(data.current)
        break
      }
      case 'audio.deviceChanged': {
        if (data.current !== undefined) setCurrentDevice(data.current)
        break
      }
    }
  }

  return (
    <AudioDevicesContext.Provider
      value={{ deviceTypes, currentDevice, fetchDevices, setAudioDevice }}
    >
      {children}
    </AudioDevicesContext.Provider>
  )
}

export const useAudioDevices = () => {
  const context = useContext(AudioDevicesContext)
  if (context === undefined)
    throw new Error(
      'useAudioDevices must be used within an AudioDevicesProvider',
    )
  return context
}
