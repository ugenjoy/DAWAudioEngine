export const ClipType = {
  Audio: 'AudioClip',
  Midi: 'MidiClip',
} as const

export type ClipType = (typeof ClipType)[keyof typeof ClipType]

export interface AudioClip {
  type: typeof ClipType.Audio
  id: string
  name: string
  position: number
  duration: number
  offset: number
  fileName: string
  gain: number
  fadeIn: number
  fadeOut: number
  waveform: number[] // Interleaved [min, max, min, max, ...] at 200 points/sec
}

export interface MidiClip {
  type: typeof ClipType.Midi
  id: string
  name: string
  position: number
  duration: number
  offset: number
  fileName: string
}

export type Clip = AudioClip | MidiClip
