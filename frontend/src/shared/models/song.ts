import { Track } from './track'

export interface Song {
  id: string
  name: string
  tempo: number
  tracks: Track[]
}
