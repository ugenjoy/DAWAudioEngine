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
  | 'transport.setMasterVolume'

type AudioCommand = 'audio.listInputs' | 'audio.listDevices' | 'audio.setDevice'

type TrackCommand =
  | 'track.setInput'
  | 'track.setMonitoring'
  | 'track.setMute'
  | 'track.setSolo'
  | 'track.setVolume'
  | 'track.add'
  | 'track.remove'
  | 'track.rename'
  | 'track.reorder'
  | 'track.setColor'

type SongCommand = 'song.setTempo' | 'song.setMetronomeMute' | 'song.create' | 'song.rename' | 'song.reorder' | 'song.setEndPosition' | 'song.delete'

type ModeCommand = 'mode.setEdit' | 'mode.setLive' | 'mode.getMode'

type MidiCommand = 'midi.listOutputs' | 'midi.send'

type ClipCommand = 'clip.add' | 'clip.move' | 'clip.remove'

type EventCommand = 'event.list' | 'event.add' | 'event.remove' | 'event.update'

type SetlistCommand =
  | 'setlist.list'
  | 'setlist.create'
  | 'setlist.update'
  | 'setlist.delete'
  | 'setlist.load'
  | 'setlist.loadSingle'
  | 'setlist.unload'
  | 'setlist.advance'
  | 'setlist.previous'
  | 'setlist.goTo'

type Command =
  | ProjectCommand
  | TransportCommand
  | AudioCommand
  | TrackCommand
  | SongCommand
  | ModeCommand
  | MidiCommand
  | EventCommand
  | ClipCommand
  | SetlistCommand

export interface WebSocketMessage {
  action: Command
  [key: string]: unknown
}
