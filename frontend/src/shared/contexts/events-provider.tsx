import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useState,
} from 'react'
import { useWebSocket } from './websocket-provider'
import { EventRule, EventAction } from '../models/event-rule'

export interface MidiDevice {
  name: string
  identifier: string
}

type EventsProviderState = {
  projectEvents: EventRule[]
  songEvents: EventRule[]
  midiOutputs: MidiDevice[]
  midiInputs: MidiDevice[]
  addEvent: (scope: 'project' | 'song', trigger: string, triggerParams: Record<string, unknown>, eventAction: EventAction) => void
  removeEvent: (scope: 'project' | 'song', eventId: string) => void
  updateEvent: (scope: 'project' | 'song', eventId: string, changes: { trigger?: string; triggerParams?: Record<string, unknown>; eventAction?: EventAction; enabled?: boolean }) => void
  fetchMidiOutputs: () => void
  fetchMidiInputs: () => void
  fetchEvents: () => void
}

const initialState: EventsProviderState = {
  projectEvents: [],
  songEvents: [],
  midiOutputs: [],
  midiInputs: [],
  addEvent: () => null,
  removeEvent: () => null,
  updateEvent: () => null,
  fetchMidiOutputs: () => null,
  fetchMidiInputs: () => null,
  fetchEvents: () => null,
}

const EventsContext = createContext<EventsProviderState>(initialState)

export function EventsProvider({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  const { ws, isConnected, send } = useWebSocket()
  const [projectEvents, setProjectEvents] = useState<EventRule[]>([])
  const [songEvents, setSongEvents] = useState<EventRule[]>([])
  const [midiOutputs, setMidiOutputs] = useState<MidiDevice[]>([])
  const [midiInputs, setMidiInputs] = useState<MidiDevice[]>([])

  const fetchMidiOutputs = useCallback(() => {
    if (ws && isConnected) {
      send({ action: 'midi.listOutputs' })
    }
  }, [ws, isConnected, send])

  const fetchMidiInputs = useCallback(() => {
    if (ws && isConnected) {
      send({ action: 'midi.listInputs' })
    }
  }, [ws, isConnected, send])

  const fetchEvents = useCallback(() => {
    if (ws && isConnected) {
      send({ action: 'event.list' })
    }
  }, [ws, isConnected, send])

  const addEvent = useCallback(
    (scope: 'project' | 'song', trigger: string, triggerParams: Record<string, unknown>, eventAction: EventAction) => {
      send({ action: 'event.add', scope, trigger, triggerParams, eventAction })
    },
    [send],
  )

  const removeEvent = useCallback(
    (scope: 'project' | 'song', eventId: string) => {
      send({ action: 'event.remove', scope, eventId })
    },
    [send],
  )

  const updateEvent = useCallback(
    (
      scope: 'project' | 'song',
      eventId: string,
      changes: { trigger?: string; triggerParams?: Record<string, unknown>; eventAction?: EventAction; enabled?: boolean },
    ) => {
      send({ action: 'event.update', scope, eventId, ...changes })
    },
    [send],
  )

  useEffect(() => {
    if (ws && isConnected) {
      ws.addEventListener('message', onMessage)
      return () => ws.removeEventListener('message', onMessage)
    }
  }, [ws, isConnected])

  function onMessage(ev: MessageEvent<unknown>) {
    if (typeof ev.data !== 'string') return
    const data = JSON.parse(ev.data)
    switch (data.event) {
      case 'event.listUpdated': {
        if (data.projectEvents !== undefined)
          setProjectEvents(data.projectEvents)
        if (data.songEvents !== undefined) setSongEvents(data.songEvents)
        break
      }
      case 'midi.outputsListed': {
        if (data.outputs !== undefined) setMidiOutputs(data.outputs)
        break
      }
      case 'midi.inputsListed': {
        if (data.inputs !== undefined) setMidiInputs(data.inputs)
        break
      }
    }
  }

  return (
    <EventsContext.Provider
      value={{
        projectEvents,
        songEvents,
        midiOutputs,
        midiInputs,
        addEvent,
        removeEvent,
        updateEvent,
        fetchMidiOutputs,
        fetchMidiInputs,
        fetchEvents,
      }}
    >
      {children}
    </EventsContext.Provider>
  )
}

export const useEvents = () => {
  const context = useContext(EventsContext)
  if (context === undefined)
    throw new Error('useEvents must be used within an EventsProvider')
  return context
}
