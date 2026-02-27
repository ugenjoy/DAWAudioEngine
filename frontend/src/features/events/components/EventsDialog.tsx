import { useState } from 'react'
import { useEvents, MidiDevice } from '@/shared/contexts/events-provider'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { useMode } from '@/shared/contexts/mode-provider'
import { EventRule } from '@/shared/models/event-rule'
import { Button } from '@/shared/shadcn/components/button'
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
} from '@/shared/shadcn/components/dialog'
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from '@/shared/shadcn/components/select'
import { Label } from '@/shared/shadcn/components/label'
import { Input } from '@/shared/shadcn/components/input'
import { Switch } from '@/shared/shadcn/components/switch'
import { IconBolt, IconPlus, IconTrash } from '@tabler/icons-react'

// ── MIDI message helpers ───────────────────────────────────────────────────

type MidiMessageType = 'programChange' | 'controlChange' | 'noteOn' | 'noteOff'

const MIDI_MESSAGE_TYPES: { value: MidiMessageType; label: string }[] = [
  { value: 'programChange', label: 'Program Change' },
  { value: 'controlChange', label: 'Control Change' },
  { value: 'noteOn', label: 'Note On' },
  { value: 'noteOff', label: 'Note Off' },
]

function buildMidiBytes(
  type: MidiMessageType,
  channel: number,
  data1: number,
  data2: number,
): number[] {
  const ch = Math.max(0, Math.min(15, channel - 1))
  switch (type) {
    case 'programChange':
      return [0xc0 | ch, data1]
    case 'controlChange':
      return [0xb0 | ch, data1, data2]
    case 'noteOn':
      return [0x90 | ch, data1, data2]
    case 'noteOff':
      return [0x80 | ch, data1, data2]
  }
}

function parseMidiBytes(bytes: number[]): {
  type: MidiMessageType
  channel: number
  data1: number
  data2: number
} {
  if (!bytes || bytes.length === 0)
    return { type: 'programChange', channel: 1, data1: 0, data2: 0 }

  const statusByte = bytes[0]
  const nibble = statusByte & 0xf0
  const ch = (statusByte & 0x0f) + 1

  if (nibble === 0xc0) {
    return {
      type: 'programChange',
      channel: ch,
      data1: bytes[1] ?? 0,
      data2: 0,
    }
  } else if (nibble === 0xb0) {
    return {
      type: 'controlChange',
      channel: ch,
      data1: bytes[1] ?? 0,
      data2: bytes[2] ?? 0,
    }
  } else if (nibble === 0x90) {
    return {
      type: 'noteOn',
      channel: ch,
      data1: bytes[1] ?? 0,
      data2: bytes[2] ?? 127,
    }
  } else if (nibble === 0x80) {
    return {
      type: 'noteOff',
      channel: ch,
      data1: bytes[1] ?? 0,
      data2: bytes[2] ?? 0,
    }
  }
  return { type: 'programChange', channel: 1, data1: 0, data2: 0 }
}

// ── EventRuleRow ───────────────────────────────────────────────────────────

interface EventRuleRowProps {
  rule: EventRule
  scope: 'project' | 'song'
  midiOutputs: MidiDevice[]
}

function EventRuleRow({
  rule,
  scope,
  midiOutputs,
}: Readonly<EventRuleRowProps>) {
  const { updateEvent, removeEvent } = useEvents()
  const { send } = useWebSocket()
  const { isLiveMode } = useMode()

  const params = (rule.action.params ?? {}) as {
    device?: string
    message?: number[]
  }
  const device = params.device ?? ''
  const parsed = parseMidiBytes(params.message ?? [])

  // Build a complete update, always including all fields to avoid backend reset
  function sendUpdate(
    trigger: string,
    enabled: boolean,
    msgDevice: string,
    type: MidiMessageType,
    channel: number,
    data1: number,
    data2: number,
  ) {
    updateEvent(scope, rule.id, {
      trigger,
      enabled,
      eventAction: {
        type: 'midi.send',
        params: {
          device: msgDevice,
          message: buildMidiBytes(type, channel, data1, data2),
        },
      },
    })
  }

  function handleToggle(enabled: boolean) {
    sendUpdate(
      rule.trigger,
      enabled,
      device,
      parsed.type,
      parsed.channel,
      parsed.data1,
      parsed.data2,
    )
  }

  function handleTriggerChange(trigger: string) {
    sendUpdate(
      trigger,
      rule.enabled,
      device,
      parsed.type,
      parsed.channel,
      parsed.data1,
      parsed.data2,
    )
  }

  function handleDeviceChange(newDevice: string) {
    sendUpdate(
      rule.trigger,
      rule.enabled,
      newDevice,
      parsed.type,
      parsed.channel,
      parsed.data1,
      parsed.data2,
    )
  }

  function handleMidiTypeChange(type: MidiMessageType) {
    sendUpdate(
      rule.trigger,
      rule.enabled,
      device,
      type,
      parsed.channel,
      parsed.data1,
      parsed.data2,
    )
  }

  function handleChannelChange(value: string) {
    const ch = Number(value)
    if (isNaN(ch)) return
    sendUpdate(
      rule.trigger,
      rule.enabled,
      device,
      parsed.type,
      ch,
      parsed.data1,
      parsed.data2,
    )
  }

  function handleData1Change(value: string) {
    const d = Number(value)
    if (isNaN(d)) return
    sendUpdate(
      rule.trigger,
      rule.enabled,
      device,
      parsed.type,
      parsed.channel,
      d,
      parsed.data2,
    )
  }

  function handleData2Change(value: string) {
    const d = Number(value)
    if (isNaN(d)) return
    sendUpdate(
      rule.trigger,
      rule.enabled,
      device,
      parsed.type,
      parsed.channel,
      parsed.data1,
      d,
    )
  }

  function handleTest() {
    if (!device || !params.message) return
    send({ action: 'midi.send', device, message: params.message })
  }

  const needsData2 =
    parsed.type === 'controlChange' ||
    parsed.type === 'noteOn' ||
    parsed.type === 'noteOff'

  return (
    <div className="flex flex-col gap-2 border rounded p-3 text-sm">
      <div className="flex items-center justify-between gap-2">
        <Switch
          checked={rule.enabled}
          onCheckedChange={handleToggle}
          disabled={isLiveMode}
        />

        {/* Trigger */}
        <Select
          value={rule.trigger}
          onValueChange={handleTriggerChange}
          disabled={isLiveMode}
        >
          <SelectTrigger className="w-36">
            <SelectValue placeholder="Trigger" />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="song.loaded">Song Loaded</SelectItem>
          </SelectContent>
        </Select>

        <span className="text-muted-foreground">→</span>

        {/* Action type (fixed midi.send for now) */}
        <span className="text-xs text-muted-foreground">MIDI Send</span>

        <div className="flex-1" />

        <Button
          variant="ghost"
          size="icon-sm"
          onClick={handleTest}
          title="Test"
        >
          <IconBolt size={14} />
        </Button>
        <Button
          variant="ghost"
          size="icon-sm"
          onClick={() => removeEvent(scope, rule.id)}
          disabled={isLiveMode}
        >
          <IconTrash size={14} />
        </Button>
      </div>

      <div className="flex flex-wrap gap-2 items-end">
        {/* MIDI device */}
        <div className="flex flex-col gap-1">
          <Label className="text-xs">Device</Label>
          <Select
            value={device}
            onValueChange={handleDeviceChange}
            disabled={isLiveMode}
          >
            <SelectTrigger className="w-44">
              <SelectValue placeholder="Select device" />
            </SelectTrigger>
            <SelectContent>
              {midiOutputs.map((d) => (
                <SelectItem key={d.identifier} value={d.identifier}>
                  {d.name}
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
        </div>

        {/* Message type */}
        <div className="flex flex-col gap-1">
          <Label className="text-xs">Type</Label>
          <Select
            value={parsed.type}
            onValueChange={(v) => handleMidiTypeChange(v as MidiMessageType)}
            disabled={isLiveMode}
          >
            <SelectTrigger className="w-36">
              <SelectValue />
            </SelectTrigger>
            <SelectContent>
              {MIDI_MESSAGE_TYPES.map((t) => (
                <SelectItem key={t.value} value={t.value}>
                  {t.label}
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
        </div>

        {/* Channel */}
        <div className="flex flex-col gap-1">
          <Label className="text-xs">Channel</Label>
          <Input
            type="number"
            min={1}
            max={16}
            value={parsed.channel}
            onChange={(e) => handleChannelChange(e.target.value)}
            className="w-16"
            disabled={isLiveMode}
          />
        </div>

        {/* Data1 (program, note, CC#) */}
        <div className="flex flex-col gap-1">
          <Label className="text-xs">
            {parsed.type === 'programChange'
              ? 'Program'
              : parsed.type === 'controlChange'
                ? 'CC#'
                : 'Note'}
          </Label>
          <Input
            type="number"
            min={0}
            max={127}
            value={parsed.data1}
            onChange={(e) => handleData1Change(e.target.value)}
            className="w-16"
            disabled={isLiveMode}
          />
        </div>

        {/* Data2 (value / velocity) */}
        {needsData2 && (
          <div className="flex flex-col gap-1">
            <Label className="text-xs">
              {parsed.type === 'controlChange' ? 'Value' : 'Velocity'}
            </Label>
            <Input
              type="number"
              min={0}
              max={127}
              value={parsed.data2}
              onChange={(e) => handleData2Change(e.target.value)}
              className="w-16"
              disabled={isLiveMode}
            />
          </div>
        )}
      </div>
    </div>
  )
}

// ── EventsDialog ───────────────────────────────────────────────────────────

export function EventsDialog() {
  const {
    projectEvents,
    songEvents,
    midiOutputs,
    addEvent,
    fetchMidiOutputs,
    fetchEvents,
  } = useEvents()
  const { isLiveMode } = useMode()
  const [open, setOpen] = useState(false)

  function handleOpen(v: boolean) {
    setOpen(v)
    if (v) {
      fetchMidiOutputs()
      fetchEvents()
    }
  }

  function handleAddEvent(scope: 'project' | 'song') {
    addEvent(scope, 'song.loaded', {
      type: 'midi.send',
      params: {
        device: midiOutputs[0]?.identifier ?? '',
        message: [0xc0, 0], // Program Change ch1, program 0
      },
    })
  }

  return (
    <Dialog open={open} onOpenChange={handleOpen}>
      <DialogTrigger asChild>
        <Button
          variant="ghost"
          disabled={isLiveMode}
          size="icon-sm"
          title="Events"
        >
          <IconBolt size={16} />
        </Button>
      </DialogTrigger>
      <DialogContent className="max-w-2xl max-h-[80vh] overflow-y-auto">
        <DialogHeader>
          <DialogTitle>Events</DialogTitle>
        </DialogHeader>

        <div className="flex flex-col gap-6">
          {/* Project-level events */}
          <section className="flex flex-col gap-2">
            <div className="flex items-center justify-between">
              <h3 className="text-sm font-semibold text-muted-foreground uppercase tracking-wide">
                Project events
              </h3>
              <Button
                variant="ghost"
                size="sm"
                onClick={() => handleAddEvent('project')}
                className="gap-1 text-xs"
                disabled={isLiveMode}
              >
                <IconPlus size={12} />
                Add
              </Button>
            </div>
            {projectEvents.length === 0 ? (
              <p className="text-xs text-muted-foreground italic">
                No project events yet.
              </p>
            ) : (
              projectEvents.map((rule) => (
                <EventRuleRow
                  key={rule.id}
                  rule={rule}
                  scope="project"
                  midiOutputs={midiOutputs}
                />
              ))
            )}
          </section>

          {/* Song-level events */}
          <section className="flex flex-col gap-2">
            <div className="flex items-center justify-between">
              <h3 className="text-sm font-semibold text-muted-foreground uppercase tracking-wide">
                Song events
              </h3>
              <Button
                variant="ghost"
                size="sm"
                onClick={() => handleAddEvent('song')}
                className="gap-1 text-xs"
                disabled={isLiveMode}
              >
                <IconPlus size={12} />
                Add
              </Button>
            </div>
            {songEvents.length === 0 ? (
              <p className="text-xs text-muted-foreground italic">
                No song events yet.
              </p>
            ) : (
              songEvents.map((rule) => (
                <EventRuleRow
                  key={rule.id}
                  rule={rule}
                  scope="song"
                  midiOutputs={midiOutputs}
                />
              ))
            )}
          </section>
        </div>
      </DialogContent>
    </Dialog>
  )
}
