import { useState } from 'react'
import { useEvents, MidiDevice } from '@/shared/contexts/events-provider'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { useMode } from '@/shared/contexts/mode-provider'
import { EventRule, EventAction } from '@/shared/models/event-rule'
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

// ── Types ──────────────────────────────────────────────────────────────────

type TriggerType = 'song.loaded' | 'midi.note' | 'midi.cc'
type ActionType =
  | 'midi.send'
  | 'transport.play'
  | 'transport.pause'
  | 'transport.stop'
  | 'setlist.next'
  | 'setlist.prev'
  | 'loop.cancel'
  | 'loop.exit'

const TRIGGER_TYPES: { value: TriggerType; label: string }[] = [
  { value: 'song.loaded', label: 'Song Loaded' },
  { value: 'midi.note',   label: 'MIDI Note' },
  { value: 'midi.cc',     label: 'MIDI CC' },
]

const ACTION_TYPES: { value: ActionType; label: string }[] = [
  { value: 'midi.send',       label: 'MIDI Send' },
  { value: 'transport.play',  label: 'Play' },
  { value: 'transport.pause', label: 'Pause' },
  { value: 'transport.stop',  label: 'Stop' },
  { value: 'setlist.next',    label: 'Next Song' },
  { value: 'setlist.prev',    label: 'Prev Song' },
  { value: 'loop.cancel',     label: 'Cancel Loop' },
  { value: 'loop.exit',       label: 'Exit Loop' },
]

// ── MIDI message helpers ───────────────────────────────────────────────────

type MidiMessageType = 'programChange' | 'controlChange' | 'noteOn' | 'noteOff'

const MIDI_MESSAGE_TYPES: { value: MidiMessageType; label: string }[] = [
  { value: 'programChange', label: 'Program Change' },
  { value: 'controlChange', label: 'Control Change' },
  { value: 'noteOn',        label: 'Note On' },
  { value: 'noteOff',       label: 'Note Off' },
]

function buildMidiBytes(
  type: MidiMessageType,
  channel: number,
  data1: number,
  data2: number,
): number[] {
  const ch = Math.max(0, Math.min(15, channel - 1))
  switch (type) {
    case 'programChange': return [0xc0 | ch, data1]
    case 'controlChange': return [0xb0 | ch, data1, data2]
    case 'noteOn':        return [0x90 | ch, data1, data2]
    case 'noteOff':       return [0x80 | ch, data1, data2]
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

  if (nibble === 0xc0)
    return { type: 'programChange', channel: ch, data1: bytes[1] ?? 0, data2: 0 }
  if (nibble === 0xb0)
    return { type: 'controlChange', channel: ch, data1: bytes[1] ?? 0, data2: bytes[2] ?? 0 }
  if (nibble === 0x90)
    return { type: 'noteOn', channel: ch, data1: bytes[1] ?? 0, data2: bytes[2] ?? 127 }
  if (nibble === 0x80)
    return { type: 'noteOff', channel: ch, data1: bytes[1] ?? 0, data2: bytes[2] ?? 0 }

  return { type: 'programChange', channel: 1, data1: 0, data2: 0 }
}

// ── EventRuleRow ───────────────────────────────────────────────────────────

interface EventRuleRowProps {
  rule: EventRule
  scope: 'project' | 'song'
  midiOutputs: MidiDevice[]
  midiInputs: MidiDevice[]
}

function EventRuleRow({
  rule,
  scope,
  midiOutputs,
  midiInputs,
}: Readonly<EventRuleRowProps>) {
  const { updateEvent, removeEvent } = useEvents()
  const { send } = useWebSocket()
  const { isLiveMode } = useMode()

  // ── Derive state from rule prop ──────────────────────────────────────────
  const triggerType = (rule.trigger ?? 'song.loaded') as TriggerType
  const tp = (rule.triggerParams ?? {}) as Record<string, unknown>
  const tpDevice    = (tp.device    as string)  ?? ''
  const tpChannel   = (tp.channel   as number)  ?? 0
  const tpNote      = (tp.note      as number)  ?? 60
  const tpCc        = (tp.cc        as number)  ?? 0
  const tpThreshold = (tp.threshold as number)  ?? 0

  const actionType = (rule.action.type ?? 'midi.send') as ActionType
  const midiParams = (rule.action.params ?? {}) as { device?: string; message?: number[] }
  const midiDevice = midiParams.device ?? ''
  const parsed     = parseMidiBytes(midiParams.message ?? [])

  // ── Full update sender ───────────────────────────────────────────────────
  function sendUpdate(opts: {
    trigger: TriggerType
    triggerParams: Record<string, unknown>
    actionType: ActionType
    midiDevice: string
    midiMsgType: MidiMessageType
    channel: number
    data1: number
    data2: number
    enabled: boolean
  }) {
    const eventAction: EventAction =
      opts.actionType === 'midi.send'
        ? {
            type: 'midi.send',
            params: {
              device: opts.midiDevice,
              message: buildMidiBytes(opts.midiMsgType, opts.channel, opts.data1, opts.data2),
            },
          }
        : { type: opts.actionType, params: {} }

    updateEvent(scope, rule.id, {
      trigger: opts.trigger,
      triggerParams: opts.triggerParams,
      eventAction,
      enabled: opts.enabled,
    })
  }

  function currentOpts() {
    return {
      trigger: triggerType,
      triggerParams: tp,
      actionType,
      midiDevice,
      midiMsgType: parsed.type,
      channel: parsed.channel,
      data1: parsed.data1,
      data2: parsed.data2,
      enabled: rule.enabled,
    }
  }

  // ── Handlers ─────────────────────────────────────────────────────────────
  function handleToggle(enabled: boolean) {
    sendUpdate({ ...currentOpts(), enabled })
  }

  function handleTriggerTypeChange(t: TriggerType) {
    const defaults: Record<TriggerType, Record<string, unknown>> = {
      'song.loaded': {},
      'midi.note':   { device: '', channel: 0, note: 60 },
      'midi.cc':     { device: '', channel: 0, cc: 0, threshold: 0 },
    }
    sendUpdate({ ...currentOpts(), trigger: t, triggerParams: defaults[t] })
  }

  function handleTriggerParam(key: string, value: string | number) {
    sendUpdate({ ...currentOpts(), triggerParams: { ...tp, [key]: value } })
  }

  function handleActionTypeChange(a: ActionType) {
    sendUpdate({ ...currentOpts(), actionType: a })
  }

  function handleDeviceChange(newDevice: string) {
    sendUpdate({ ...currentOpts(), midiDevice: newDevice })
  }

  function handleMidiTypeChange(t: MidiMessageType) {
    sendUpdate({ ...currentOpts(), midiMsgType: t })
  }

  function handleChannelChange(v: string) {
    const ch = Number(v)
    if (!isNaN(ch)) sendUpdate({ ...currentOpts(), channel: ch })
  }

  function handleData1Change(v: string) {
    const d = Number(v)
    if (!isNaN(d)) sendUpdate({ ...currentOpts(), data1: d })
  }

  function handleData2Change(v: string) {
    const d = Number(v)
    if (!isNaN(d)) sendUpdate({ ...currentOpts(), data2: d })
  }

  function handleTest() {
    if (!midiDevice || !midiParams.message) return
    send({ action: 'midi.send', device: midiDevice, message: midiParams.message })
  }

  const needsData2 =
    parsed.type === 'controlChange' ||
    parsed.type === 'noteOn' ||
    parsed.type === 'noteOff'

  return (
    <div className="flex flex-col gap-2 border rounded p-3 text-sm">
      {/* Row 1: enabled + trigger type + arrow + action type + test + delete */}
      <div className="flex items-center justify-between gap-2 flex-wrap">
        <Switch
          checked={rule.enabled}
          onCheckedChange={handleToggle}
          disabled={isLiveMode}
        />

        <Select
          value={triggerType}
          onValueChange={(v) => handleTriggerTypeChange(v as TriggerType)}
          disabled={isLiveMode}
        >
          <SelectTrigger className="w-32">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            {TRIGGER_TYPES.map((t) => (
              <SelectItem key={t.value} value={t.value}>{t.label}</SelectItem>
            ))}
          </SelectContent>
        </Select>

        <span className="text-muted-foreground">→</span>

        <Select
          value={actionType}
          onValueChange={(v) => handleActionTypeChange(v as ActionType)}
          disabled={isLiveMode}
        >
          <SelectTrigger className="w-36">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            {ACTION_TYPES.map((a) => (
              <SelectItem key={a.value} value={a.value}>{a.label}</SelectItem>
            ))}
          </SelectContent>
        </Select>

        <div className="flex-1" />

        {actionType === 'midi.send' && (
          <Button variant="ghost" size="icon-sm" onClick={handleTest} title="Test">
            <IconBolt size={14} />
          </Button>
        )}
        <Button
          variant="ghost"
          size="icon-sm"
          onClick={() => removeEvent(scope, rule.id)}
          disabled={isLiveMode}
        >
          <IconTrash size={14} />
        </Button>
      </div>

      {/* Row 2: trigger params (midi.note) */}
      {triggerType === 'midi.note' && (
        <div className="flex flex-wrap gap-2 items-end">
          <div className="flex flex-col gap-1">
            <Label className="text-xs">Input Device</Label>
            <Select
              value={tpDevice}
              onValueChange={(v) => handleTriggerParam('device', v)}
              disabled={isLiveMode}
            >
              <SelectTrigger className="w-44">
                <SelectValue placeholder="Any device" />
              </SelectTrigger>
              <SelectContent>
                <SelectItem value="">Any device</SelectItem>
                {midiInputs.map((d) => (
                  <SelectItem key={d.identifier} value={d.name}>{d.name}</SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>
          <div className="flex flex-col gap-1">
            <Label className="text-xs">Channel</Label>
            <Input
              type="number" min={0} max={16}
              value={tpChannel}
              onChange={(e) => handleTriggerParam('channel', Number(e.target.value))}
              className="w-16"
              disabled={isLiveMode}
              title="0 = any channel"
            />
          </div>
          <div className="flex flex-col gap-1">
            <Label className="text-xs">Note</Label>
            <Input
              type="number" min={0} max={127}
              value={tpNote}
              onChange={(e) => handleTriggerParam('note', Number(e.target.value))}
              className="w-16"
              disabled={isLiveMode}
            />
          </div>
        </div>
      )}

      {/* Row 2: trigger params (midi.cc) */}
      {triggerType === 'midi.cc' && (
        <div className="flex flex-wrap gap-2 items-end">
          <div className="flex flex-col gap-1">
            <Label className="text-xs">Input Device</Label>
            <Select
              value={tpDevice}
              onValueChange={(v) => handleTriggerParam('device', v)}
              disabled={isLiveMode}
            >
              <SelectTrigger className="w-44">
                <SelectValue placeholder="Any device" />
              </SelectTrigger>
              <SelectContent>
                <SelectItem value="">Any device</SelectItem>
                {midiInputs.map((d) => (
                  <SelectItem key={d.identifier} value={d.name}>{d.name}</SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>
          <div className="flex flex-col gap-1">
            <Label className="text-xs">Channel</Label>
            <Input
              type="number" min={0} max={16}
              value={tpChannel}
              onChange={(e) => handleTriggerParam('channel', Number(e.target.value))}
              className="w-16"
              disabled={isLiveMode}
              title="0 = any channel"
            />
          </div>
          <div className="flex flex-col gap-1">
            <Label className="text-xs">CC#</Label>
            <Input
              type="number" min={0} max={127}
              value={tpCc}
              onChange={(e) => handleTriggerParam('cc', Number(e.target.value))}
              className="w-16"
              disabled={isLiveMode}
            />
          </div>
          <div className="flex flex-col gap-1">
            <Label className="text-xs">Threshold</Label>
            <Input
              type="number" min={0} max={127}
              value={tpThreshold}
              onChange={(e) => handleTriggerParam('threshold', Number(e.target.value))}
              className="w-16"
              disabled={isLiveMode}
              title="Fires when value ≥ threshold"
            />
          </div>
        </div>
      )}

      {/* Row 3: action params (midi.send only) */}
      {actionType === 'midi.send' && (
        <div className="flex flex-wrap gap-2 items-end">
          <div className="flex flex-col gap-1">
            <Label className="text-xs">Device</Label>
            <Select
              value={midiDevice}
              onValueChange={handleDeviceChange}
              disabled={isLiveMode}
            >
              <SelectTrigger className="w-44">
                <SelectValue placeholder="Select device" />
              </SelectTrigger>
              <SelectContent>
                {midiOutputs.map((d) => (
                  <SelectItem key={d.identifier} value={d.name}>{d.name}</SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>

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
                  <SelectItem key={t.value} value={t.value}>{t.label}</SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>

          <div className="flex flex-col gap-1">
            <Label className="text-xs">Channel</Label>
            <Input
              type="number" min={1} max={16}
              value={parsed.channel}
              onChange={(e) => handleChannelChange(e.target.value)}
              className="w-16"
              disabled={isLiveMode}
            />
          </div>

          <div className="flex flex-col gap-1">
            <Label className="text-xs">
              {parsed.type === 'programChange' ? 'Program'
                : parsed.type === 'controlChange' ? 'CC#' : 'Note'}
            </Label>
            <Input
              type="number" min={0} max={127}
              value={parsed.data1}
              onChange={(e) => handleData1Change(e.target.value)}
              className="w-16"
              disabled={isLiveMode}
            />
          </div>

          {needsData2 && (
            <div className="flex flex-col gap-1">
              <Label className="text-xs">
                {parsed.type === 'controlChange' ? 'Value' : 'Velocity'}
              </Label>
              <Input
                type="number" min={0} max={127}
                value={parsed.data2}
                onChange={(e) => handleData2Change(e.target.value)}
                className="w-16"
                disabled={isLiveMode}
              />
            </div>
          )}
        </div>
      )}
    </div>
  )
}

// ── EventsDialog ───────────────────────────────────────────────────────────

export function EventsDialog() {
  const {
    projectEvents,
    songEvents,
    midiOutputs,
    midiInputs,
    addEvent,
    fetchMidiOutputs,
    fetchMidiInputs,
    fetchEvents,
  } = useEvents()
  const { isLiveMode } = useMode()
  const [open, setOpen] = useState(false)

  function handleOpen(v: boolean) {
    setOpen(v)
    if (v) {
      fetchMidiOutputs()
      fetchMidiInputs()
      fetchEvents()
    }
  }

  function handleAddEvent(scope: 'project' | 'song') {
    addEvent(scope, 'song.loaded', {}, {
      type: 'midi.send',
      params: {
        device: midiOutputs[0]?.name ?? '',
        message: [0xc0, 0],
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
                  midiInputs={midiInputs}
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
                  midiInputs={midiInputs}
                />
              ))
            )}
          </section>
        </div>
      </DialogContent>
    </Dialog>
  )
}
