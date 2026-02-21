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
}

export type Track = BeatTrack | AudioFileTrack
