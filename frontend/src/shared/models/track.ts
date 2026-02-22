import { Clip } from './clip'

export const TrackType = {
  BeatTrack: 'BeatTrack',
  AudioFileTrack: 'AudioFileTrack',
} as const

export type TrackType = (typeof TrackType)[keyof typeof TrackType]

export interface BeatTrack {
  type: typeof TrackType.BeatTrack
  id: string
  name: string
  volume: number
  pan: number
  mute: boolean
  solo: boolean
  frequency: number
  inputChannel: number
  inputStereo: boolean
  monitoring: boolean
}

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
}

export type Track = BeatTrack | AudioFileTrack
