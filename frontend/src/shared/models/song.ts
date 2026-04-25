import { EventRule } from './event-rule'
import { Loop } from './loop'
import { Track } from './track'

export interface Song {
  id: string
  name: string
  tempo: number
  metronomeMute: boolean
  tracks: Track[]
  events?: EventRule[]
  endPosition?: number
  loops?: Loop[]
}
