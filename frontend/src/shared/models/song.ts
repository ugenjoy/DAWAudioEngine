import { EventRule } from './event-rule'
import { Track } from './track'

export interface Song {
  id: string
  name: string
  tempo: number
  metronomeMute: boolean
  tracks: Track[]
  events?: EventRule[]
}
