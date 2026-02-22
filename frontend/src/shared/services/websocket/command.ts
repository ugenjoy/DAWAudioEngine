type ProjectCommand =
  | 'project.load'
  | 'project.save'
  | 'project.getLoaded'
  | 'project.list'
  | 'project.loadSong'

type TransportCommand =
  | 'transport.play'
  | 'transport.stop'
  | 'transport.pause'
  | 'transport.setPlayheadPosition'
  | 'transport.setCursorPosition'

type AudioCommand = 'audio.listInputs' | 'audio.listDevices' | 'audio.setDevice'

type TrackCommand = 'track.setInput' | 'track.setMonitoring'

type Command = ProjectCommand | TransportCommand | AudioCommand | TrackCommand

export interface WebSocketMessage {
  action: Command
  [key: string]: unknown
}
