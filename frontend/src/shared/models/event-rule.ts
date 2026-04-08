export interface MidiSendParams {
  device: string
  message: number[] // Raw MIDI bytes, e.g. [0xC0, 42] for program change ch1
}

export interface SeekToPositionParams {
  markerId: string
}

export interface EventAction {
  type: 'midi.send' | 'transport.play' | 'transport.pause' | 'transport.stop'
      | 'setlist.next' | 'setlist.prev' | 'loop.cancel' | 'loop.exit'
      | 'transport.seekToPosition' | string
  params: MidiSendParams | SeekToPositionParams | Record<string, unknown>
}

export interface MidiNoteTriggerParams {
  device: string   // device name, empty = any
  channel: number  // 1-16, 0 = any
  note: number     // 0-127
}

export interface MidiCcTriggerParams {
  device: string
  channel: number
  cc: number        // 0-127
  threshold: number // fire when value >= threshold
}

export interface PositionTriggerParams {
  markerId: string // references a Marker.id
}

export interface EventRule {
  id: string
  trigger: 'song.loaded' | 'midi.note' | 'midi.cc' | 'position' | string
  triggerParams?: MidiNoteTriggerParams | MidiCcTriggerParams | PositionTriggerParams | Record<string, unknown>
  action: EventAction
  enabled: boolean
}
