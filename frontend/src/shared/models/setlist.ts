import { EventRule } from './event-rule'

export type SetlistTransition = 'stop' | 'continue'

export interface SetlistEntry {
  songId: string
  transition: SetlistTransition
}

export interface Setlist {
  id: string
  name: string
  entries: SetlistEntry[]
  events: EventRule[]
}
