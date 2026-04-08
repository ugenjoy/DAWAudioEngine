import { useState, useEffect } from 'react'
import { useEvents, MidiDevice } from '@/shared/contexts/events-provider'
import { useWebSocket } from '@/shared/contexts/websocket-provider'
import { useMode } from '@/shared/contexts/mode-provider'
import { EventRule, EventAction } from '@/shared/models/event-rule'
import { Button } from '@/shared/shadcn/components/button'
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
  AlertDialogTrigger,
} from '@/shared/shadcn/components/alert-dialog'
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

type TriggerType = 'song.loaded' | 'midi.note' | 'midi.cc' | 'position'
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
  { value: 'midi.note', label: 'MIDI Note' },
  { value: 'midi.cc', label: 'MIDI CC' },
  { value: 'position', label: 'Position' },
]

const ACTION_TYPES: { value: ActionType; label: string }[] = [
  { value: 'midi.send', label: 'MIDI Send' },
  { value: 'transport.play', label: 'Play' },
  { value: 'transport.pause', label: 'Pause' },
  { value: 'transport.stop', label: 'Stop' },
  { value: 'setlist.next', label: 'Next Song' },
  { value: 'setlist.prev', label: 'Prev Song' },
  { value: 'loop.cancel', label: 'Cancel Loop' },
  { value: 'loop.exit', label: 'Exit Loop' },
]

function triggerLabel(t: string): string {
  return TRIGGER_TYPES.find((x) => x.value === t)?.label ?? t
}
function actionLabel(a: string): string {
  return ACTION_TYPES.find((x) => x.value === a)?.label ?? a
}

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

// ── EventRuleListItem (left panel) ─────────────────────────────────────────

interface EventRuleListItemProps {
  rule: EventRule
  selected: boolean
  onSelect: () => void
}

function EventRuleListItem({ rule, selected, onSelect }: Readonly<EventRuleListItemProps>) {
  return (
    <button
      onClick={onSelect}
      className={[
        'w-full text-left px-3 py-2 rounded text-xs flex items-center gap-2 transition-colors',
        selected
          ? 'bg-accent text-accent-foreground'
          : 'hover:bg-muted text-muted-foreground hover:text-foreground',
      ].join(' ')}
    >
      <span
        className={[
          'w-1.5 h-1.5 rounded-full shrink-0',
          rule.enabled ? 'bg-green-500' : 'bg-muted-foreground/40',
        ].join(' ')}
      />
      <span className="truncate">
        {triggerLabel(rule.trigger)} → {actionLabel(rule.action.type)}
      </span>
    </button>
  )
}

// ── EventRuleDetail (right panel) ──────────────────────────────────────────

interface EventRuleDetailProps {
  rule: EventRule
  scope: 'project' | 'song'
  midiOutputs: MidiDevice[]
  midiInputs: MidiDevice[]
  onDeleted: () => void
}

function EventRuleDetail({
  rule,
  scope,
  midiOutputs,
  midiInputs,
  onDeleted,
}: Readonly<EventRuleDetailProps>) {
  const { updateEvent, removeEvent } = useEvents()
  const { send } = useWebSocket()
  const { isLiveMode } = useMode()

  const triggerType = (rule.trigger ?? 'song.loaded') as TriggerType
  const tp = (rule.triggerParams ?? {}) as Record<string, unknown>
  const tpDevice    = (tp.device    as string) ?? ''
  const tpChannel   = (tp.channel   as number) ?? 0
  const tpNote      = (tp.note      as number) ?? 60
  const tpCc        = (tp.cc        as number) ?? 0
  const tpThreshold = (tp.threshold as number) ?? 0
  const tpPosition  = (tp.position  as number) ?? 0

  const actionType = (rule.action.type ?? 'midi.send') as ActionType
  const midiParams = (rule.action.params ?? {}) as { device?: string; message?: number[] }
  const midiDevice = midiParams.device ?? ''
  const parsed     = parseMidiBytes(midiParams.message ?? [])

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

  function handleToggle(enabled: boolean) {
    sendUpdate({ ...currentOpts(), enabled })
  }

  function handleTriggerTypeChange(t: TriggerType) {
    const defaults: Record<TriggerType, Record<string, unknown>> = {
      'song.loaded': {},
      'midi.note':   { device: '', channel: 0, note: 60 },
      'midi.cc':     { device: '', channel: 0, cc: 0, threshold: 0 },
      'position':    { position: 0 },
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

  function handleDelete() {
    removeEvent(scope, rule.id)
    onDeleted()
  }

  const needsData2 =
    parsed.type === 'controlChange' ||
    parsed.type === 'noteOn' ||
    parsed.type === 'noteOff'

  return (
    <div className="flex flex-col gap-6 p-6">
      {/* Enabled + Delete */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2">
          <Switch
            checked={rule.enabled}
            onCheckedChange={handleToggle}
            disabled={isLiveMode}
          />
          <span className="text-sm text-muted-foreground">
            {rule.enabled ? 'Enabled' : 'Disabled'}
          </span>
        </div>

        <AlertDialog>
          <AlertDialogTrigger asChild>
            <Button variant="ghost" size="icon-sm" disabled={isLiveMode} title="Delete event">
              <IconTrash size={14} />
            </Button>
          </AlertDialogTrigger>
          <AlertDialogContent>
            <AlertDialogHeader>
              <AlertDialogTitle>Delete event?</AlertDialogTitle>
              <AlertDialogDescription>
                This action cannot be undone. The event rule will be permanently removed.
              </AlertDialogDescription>
            </AlertDialogHeader>
            <AlertDialogFooter>
              <AlertDialogCancel>Cancel</AlertDialogCancel>
              <AlertDialogAction onClick={handleDelete}>Delete</AlertDialogAction>
            </AlertDialogFooter>
          </AlertDialogContent>
        </AlertDialog>
      </div>

      {/* Trigger */}
      <div className="flex flex-col gap-3">
        <h4 className="text-xs font-semibold text-muted-foreground uppercase tracking-wide">
          Trigger
        </h4>
        <div className="flex flex-wrap gap-3 items-end">
          <div className="flex flex-col gap-1">
            <Label className="text-xs">Type</Label>
            <Select
              value={triggerType}
              onValueChange={(v) => handleTriggerTypeChange(v as TriggerType)}
              disabled={isLiveMode}
            >
              <SelectTrigger className="w-36">
                <SelectValue />
              </SelectTrigger>
              <SelectContent>
                {TRIGGER_TYPES.map((t) => (
                  <SelectItem key={t.value} value={t.value}>{t.label}</SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>

          {triggerType === 'midi.note' && (
            <>
              <div className="flex flex-col gap-1">
                <Label className="text-xs">Input Device</Label>
                <Select
                  value={tpDevice || '__any__'}
                  onValueChange={(v) => handleTriggerParam('device', v === '__any__' ? '' : v)}
                  disabled={isLiveMode}
                >
                  <SelectTrigger className="w-44">
                    <SelectValue placeholder="Any device" />
                  </SelectTrigger>
                  <SelectContent>
                    <SelectItem value="__any__">Any device</SelectItem>
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
            </>
          )}

          {triggerType === 'midi.cc' && (
            <>
              <div className="flex flex-col gap-1">
                <Label className="text-xs">Input Device</Label>
                <Select
                  value={tpDevice || '__any__'}
                  onValueChange={(v) => handleTriggerParam('device', v === '__any__' ? '' : v)}
                  disabled={isLiveMode}
                >
                  <SelectTrigger className="w-44">
                    <SelectValue placeholder="Any device" />
                  </SelectTrigger>
                  <SelectContent>
                    <SelectItem value="__any__">Any device</SelectItem>
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
            </>
          )}

          {triggerType === 'position' && (
            <div className="flex flex-col gap-1">
              <Label className="text-xs">Position (s)</Label>
              <Input
                type="number" min={0} step={0.01}
                value={tpPosition}
                onChange={(e) => handleTriggerParam('position', Number(e.target.value))}
                className="w-24"
                disabled={isLiveMode}
              />
            </div>
          )}
        </div>
      </div>

      {/* Action */}
      <div className="flex flex-col gap-3">
        <h4 className="text-xs font-semibold text-muted-foreground uppercase tracking-wide">
          Action
        </h4>
        <div className="flex flex-wrap gap-3 items-end">
          <div className="flex flex-col gap-1">
            <Label className="text-xs">Type</Label>
            <Select
              value={actionType}
              onValueChange={(v) => handleActionTypeChange(v as ActionType)}
              disabled={isLiveMode}
            >
              <SelectTrigger className="w-40">
                <SelectValue />
              </SelectTrigger>
              <SelectContent>
                {ACTION_TYPES.map((a) => (
                  <SelectItem key={a.value} value={a.value}>{a.label}</SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>

          {actionType === 'midi.send' && (
            <>
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

              <div className="flex flex-col gap-1 justify-end">
                <Label className="text-xs invisible">Test</Label>
                <Button variant="outline" size="sm" onClick={handleTest} title="Test">
                  <IconBolt size={14} className="mr-1" />
                  Test
                </Button>
              </div>
            </>
          )}
        </div>
      </div>
    </div>
  )
}

// ── EventsPanel (used by EventsPage) ──────────────────────────────────────

export function EventsPanel() {
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
  const { isConnected } = useWebSocket()
  const [selectedId, setSelectedId] = useState<{ id: string; scope: 'project' | 'song' } | null>(null)

  useEffect(() => {
    if (!isConnected) return
    fetchMidiOutputs()
    fetchMidiInputs()
    fetchEvents()
  }, [isConnected, fetchMidiOutputs, fetchMidiInputs, fetchEvents])

  function handleAddEvent(scope: 'project' | 'song') {
    addEvent(scope, 'song.loaded', {}, {
      type: 'midi.send',
      params: {
        device: midiOutputs[0]?.name ?? '',
        message: [0xc0, 0],
      },
    })
  }

  const selectedRule =
    selectedId?.scope === 'project'
      ? projectEvents.find((r) => r.id === selectedId.id)
      : selectedId?.scope === 'song'
      ? songEvents.find((r) => r.id === selectedId.id)
      : undefined

  return (
    <div className="flex flex-1 min-h-0">
      {/* Left panel — event list */}
      <div className="w-64 shrink-0 border-r flex flex-col overflow-hidden">
        <div className="flex-1 overflow-y-auto p-2 flex flex-col gap-4">

          <div className="flex flex-col gap-1">
            <div className="flex items-center justify-between px-1">
              <span className="text-xs font-semibold text-muted-foreground uppercase tracking-wide">
                Project
              </span>
              <Button
                variant="ghost"
                size="icon-sm"
                onClick={() => handleAddEvent('project')}
                disabled={isLiveMode}
                title="Add project event"
              >
                <IconPlus size={12} />
              </Button>
            </div>
            {projectEvents.length === 0 ? (
              <p className="text-xs text-muted-foreground italic px-3 py-1">No events yet.</p>
            ) : (
              projectEvents.map((rule) => (
                <EventRuleListItem
                  key={rule.id}
                  rule={rule}
                  selected={selectedId?.id === rule.id}
                  onSelect={() => setSelectedId({ id: rule.id, scope: 'project' })}
                />
              ))
            )}
          </div>

          <div className="flex flex-col gap-1">
            <div className="flex items-center justify-between px-1">
              <span className="text-xs font-semibold text-muted-foreground uppercase tracking-wide">
                Song
              </span>
              <Button
                variant="ghost"
                size="icon-sm"
                onClick={() => handleAddEvent('song')}
                disabled={isLiveMode}
                title="Add song event"
              >
                <IconPlus size={12} />
              </Button>
            </div>
            {songEvents.length === 0 ? (
              <p className="text-xs text-muted-foreground italic px-3 py-1">No events yet.</p>
            ) : (
              songEvents.map((rule) => (
                <EventRuleListItem
                  key={rule.id}
                  rule={rule}
                  selected={selectedId?.id === rule.id}
                  onSelect={() => setSelectedId({ id: rule.id, scope: 'song' })}
                />
              ))
            )}
          </div>

        </div>
      </div>

      {/* Right panel — event detail */}
      <div className="flex-1 overflow-y-auto">
        {selectedRule && selectedId ? (
          <EventRuleDetail
            key={selectedRule.id}
            rule={selectedRule}
            scope={selectedId.scope}
            midiOutputs={midiOutputs}
            midiInputs={midiInputs}
            onDeleted={() => setSelectedId(null)}
          />
        ) : (
          <div className="flex items-center justify-center h-full text-sm text-muted-foreground">
            Select an event to configure it.
          </div>
        )}
      </div>
    </div>
  )
}
