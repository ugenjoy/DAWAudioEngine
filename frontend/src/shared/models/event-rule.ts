export interface MidiSendParams {
  device: string
  message: number[] // Raw MIDI bytes, e.g. [0xC0, 42] for program change ch1
}

export interface EventAction {
  type: 'midi.send' | string // Extensible to osc.send, http.request, etc.
  params: MidiSendParams | Record<string, unknown>
}

export interface EventRule {
  id: string
  trigger: string
  action: EventAction
  enabled: boolean
}
