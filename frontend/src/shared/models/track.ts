import { Clip } from './clip'

export const TrackType = {
  AudioFileTrack: 'AudioFileTrack',
} as const

export type TrackType = (typeof TrackType)[keyof typeof TrackType]

export interface AudioFileTrack {
  type: typeof TrackType.AudioFileTrack
  id: string
  name: string
  volume: number
  pan: number
  mute: boolean
  solo: boolean
  clips: Clip[]
  inputChannel: number
  inputStereo: boolean
  monitoring: boolean
  color: number
}

export type Track = AudioFileTrack
