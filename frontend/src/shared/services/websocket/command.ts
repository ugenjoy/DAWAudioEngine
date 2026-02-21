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

type Command = ProjectCommand | TransportCommand

export interface WebSocketMessage {
  action: Command
  [key: string]: unknown
}
