export interface AudioInput {
  index: number
  name: string
}

export interface AudioDeviceType {
  name: string
  outputDevices: string[]
  inputDevices: string[]
}

export interface AudioDeviceInfo {
  deviceType: string
  outputDevice: string
  inputDevice: string
  sampleRate: number
  bufferSize: number
  availableSampleRates: number[]
  availableBufferSizes: number[]
}
