export interface MidiSendParams {
  device: string
  message: number[] // Raw MIDI bytes, e.g. [0xC0, 42] for program change ch1
}

export interface EventAction {
  type: 'midi.send' | 'transport.play' | 'transport.pause' | 'transport.stop'
      | 'setlist.next' | 'setlist.prev' | 'loop.cancel' | 'loop.exit' | string
  params: MidiSendParams | Record<string, unknown>
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

export interface EventRule {
  id: string
  trigger: 'song.loaded' | 'midi.note' | 'midi.cc' | string
  triggerParams?: MidiNoteTriggerParams | MidiCcTriggerParams | Record<string, unknown>
  action: EventAction
  enabled: boolean
}
