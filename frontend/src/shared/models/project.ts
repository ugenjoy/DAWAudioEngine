import { Song } from './song'

export interface Project {
  id: string
  name: string
  path: string
  songs: Song[]
  lastModified: string
}
