import { Button } from '@/shared/shadcn/components/button'
import { cn } from '@/shared/shadcn/lib/utils'
import { getCSSVar } from '../utils'
import { AudioInput } from '@/shared/models/audio-input'
import { useMode } from '@/shared/contexts/mode-provider'
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from '@/shared/shadcn/components/select'
import { useState, useRef, useEffect, useCallback, forwardRef } from 'react'
import { Input } from '@/shared/shadcn/components/input'

type TrackProps = {
  id: string
  name: string
  mute: boolean
  solo: boolean
  volume: number
  color: string
  height: number
  inputChannel: number
  inputStereo: boolean
  monitoring: boolean
  availableInputs: AudioInput[]
  trackLevelsRef: React.RefObject<Record<string, number>>
  onSetInput: (trackId: string, inputChannel: number, stereo: boolean) => void
  onSetMonitoring: (trackId: string, monitoring: boolean) => void
  onSetMute: (trackId: string, mute: boolean) => void
  onSetSolo: (trackId: string, solo: boolean) => void
  onSetVolume: (trackId: string, volume: number) => void
  onRename?: (trackId: string, name: string) => void
  onSetColor?: (trackId: string, color: number) => void
  onResize?: (trackId: string, height: number) => void
  selected?: boolean
  onTrackSelect?: (trackId: string) => void
}

const Track = forwardRef<
  HTMLDivElement,
  TrackProps & React.HTMLAttributes<HTMLDivElement>
>(function Track(
  {
    id,
    name,
    mute,
    solo,
    volume,
    color,
    height,
    inputChannel,
    inputStereo,
    monitoring,
    availableInputs,
    trackLevelsRef,
    onSetInput,
    onSetMonitoring,
    onSetMute,
    onSetSolo,
    onSetVolume,
    onRename,
    onSetColor,
    onResize,
    selected,
    onTrackSelect,
    ...props
  },
  ref,
) {
  const { isLiveMode } = useMode()
  const [isEditing, setIsEditing] = useState(false)
  const [editName, setEditName] = useState(name)
  const [showColorPicker, setShowColorPicker] = useState(false)
  const inputRef = useRef<HTMLInputElement>(null)
  const colorPickerRef = useRef<HTMLDivElement>(null)
  const vuMeterRef = useRef<HTMLDivElement>(null)
  const vuContainerRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    let rafId: number
    const animate = () => {
      const level = trackLevelsRef.current[id] ?? 0
      if (vuMeterRef.current) {
        // Convert linear peak to dB, then map to 0-1 using same scale as graduations
        const db = level > 0.00001 ? 20 * Math.log10(level) : -80
        const ratio = Math.max(0, Math.min(1, (db + 80) / 92))
        vuMeterRef.current.style.height = `${ratio * 100}%`
        // Color based on dB: green < -6, yellow -6..0, red > 0
        const r = db > 0 ? 255 : db > -6 ? 255 : 34
        const g = db > 0 ? Math.round(255 * Math.max(0, 1 - db / 12)) : db > -6 ? 200 : 197
        const b = db > 0 ? 0 : db > -6 ? 0 : 94
        vuMeterRef.current.style.backgroundColor = `rgb(${r},${g},${b})`
      }
      rafId = requestAnimationFrame(animate)
    }
    rafId = requestAnimationFrame(animate)
    return () => cancelAnimationFrame(rafId)
  }, [id, trackLevelsRef])

  const handleVuVolumeMouseDown = useCallback(
    (e: React.MouseEvent) => {
      if (isLiveMode) return
      e.preventDefault()
      e.stopPropagation()
      const container = vuContainerRef.current
      if (!container) return

      const updateVolume = (clientY: number) => {
        const rect = container.getBoundingClientRect()
        const ratio = Math.max(
          0,
          Math.min(1, 1 - (clientY - rect.top) / rect.height),
        )
        const db = ratio * 92 - 80 // 0 → -80 dB, 1 → +12 dB
        onSetVolume(id, Math.round(db * 2) / 2) // snap to 0.5 dB
      }

      updateVolume(e.clientY)

      const handleMouseMove = (ev: MouseEvent) => updateVolume(ev.clientY)
      const handleMouseUp = () => {
        document.removeEventListener('mousemove', handleMouseMove)
        document.removeEventListener('mouseup', handleMouseUp)
      }
      document.addEventListener('mousemove', handleMouseMove)
      document.addEventListener('mouseup', handleMouseUp)
    },
    [id, isLiveMode, onSetVolume],
  )

  const handleResizeMouseDown = useCallback(
    (e: React.MouseEvent) => {
      e.preventDefault()
      e.stopPropagation()
      const startY = e.clientY
      const startHeight = height

      const handleMouseMove = (ev: MouseEvent) => {
        const newHeight = startHeight + (ev.clientY - startY)
        onResize?.(id, newHeight)
      }

      const handleMouseUp = () => {
        document.removeEventListener('mousemove', handleMouseMove)
        document.removeEventListener('mouseup', handleMouseUp)
      }

      document.addEventListener('mousemove', handleMouseMove)
      document.addEventListener('mouseup', handleMouseUp)
    },
    [id, height, onResize],
  )

  useEffect(() => {
    if (isEditing && inputRef.current) {
      inputRef.current.focus()
      inputRef.current.select()
    }
  }, [isEditing])

  useEffect(() => {
    if (!showColorPicker) return
    function handleClickOutside(e: MouseEvent) {
      if (
        colorPickerRef.current &&
        !colorPickerRef.current.contains(e.target as Node)
      ) {
        setShowColorPicker(false)
      }
    }
    document.addEventListener('mousedown', handleClickOutside)
    return () => document.removeEventListener('mousedown', handleClickOutside)
  }, [showColorPicker])

  function handleDoubleClick() {
    if (isLiveMode) return
    setEditName(name)
    setIsEditing(true)
  }

  function commitRename() {
    setIsEditing(false)
    const trimmed = editName.trim()
    if (trimmed && trimmed !== name) {
      onRename?.(id, trimmed)
    }
  }

  function handleKeyDown(e: React.KeyboardEvent) {
    if (e.key === 'Enter') commitRename()
    if (e.key === 'Escape') setIsEditing(false)
  }

  return (
    <div
      ref={ref}
      {...props}
      className={cn(
        'relative overflow-hidden shrink-0 bg-card border border-border w-full text-xs grid grid-cols-[1fr_1rem] outline-none',
        selected && 'bg-card-selected z-10 border border-foreground/30',
        props.className,
      )}
      style={{
        height: height + 'px',
        ...props.style,
      }}
      onClick={() => onTrackSelect?.(id)}
    >
      <div className="flex flex-col gap-2 p-2 pb-4 min-w-0 [grid-area:1/1/2/2]">
        <div className="flex flex-row items-center w-full">
          <div className="flex flex-row gap-2 items-center min-w-0 w-full">
            <div className="relative" ref={colorPickerRef}>
              <button
                type="button"
                className={cn(
                  'rounded-full size-2 shrink-0 transition-transform',
                  !isLiveMode && 'hover:scale-150 cursor-pointer',
                )}
                style={{ backgroundColor: getCSSVar(color) }}
                onClick={(e) => {
                  e.stopPropagation()
                  if (!isLiveMode) setShowColorPicker((v) => !v)
                }}
                title="Change color"
              />
              {showColorPicker && (
                <div className="absolute top-4 left-0 z-50 bg-popover border border-border rounded-md p-1.5 shadow-md flex gap-1">
                  {Array.from({ length: 8 }, (_, i) => i + 1).map((c) => (
                    <button
                      key={c}
                      type="button"
                      className="rounded-full size-3 cursor-pointer hover:scale-125 transition-transform ring-offset-background focus:outline-none focus:ring-1 focus:ring-ring"
                      style={{
                        backgroundColor: getCSSVar(`--track-${c}-stroke`),
                      }}
                      onClick={(e) => {
                        e.stopPropagation()
                        onSetColor?.(id, c)
                        setShowColorPicker(false)
                      }}
                    />
                  ))}
                </div>
              )}
            </div>
            {isEditing ? (
              <input
                ref={inputRef}
                className="bg-transparent border border-border rounded px-1 w-full outline-none text-xs"
                value={editName}
                onChange={(e) => setEditName(e.target.value)}
                onBlur={commitRename}
                onKeyDown={handleKeyDown}
              />
            ) : (
              <span
                className="truncate cursor-default"
                onDoubleClick={handleDoubleClick}
              >
                {name}
              </span>
            )}
          </div>
          <div className="flex gap-0.5">
            <Button
              className={cn(
                'font-bold',
                monitoring && 'bg-green-500/20 text-green-500',
              )}
              variant="ghost"
              size="icon-xs"
              onClick={() => onSetMonitoring(id, !monitoring)}
              disabled={isLiveMode || inputChannel === -1}
              title={
                isLiveMode ? 'Not available in Live mode' : 'Input monitoring'
              }
            >
              I
            </Button>
            <Button
              className={cn(
                'font-bold',
                mute && 'bg-amber-500/20 text-amber-500',
              )}
              variant="ghost"
              size="icon-xs"
              onClick={() => onSetMute(id, !mute)}
              disabled={isLiveMode}
            >
              M
            </Button>
            <Button
              className={cn(
                'font-bold',
                solo && 'bg-blue-500/20 text-blue-500',
              )}
              variant="ghost"
              size="icon-xs"
              onClick={() => onSetSolo(id, !solo)}
              disabled={isLiveMode}
            >
              S
            </Button>
          </div>
        </div>
        <div className="flex gap-1 items-center">
          <Select
            value={String(inputChannel)}
            onValueChange={(val) => {
              onSetInput(id, Number.parseInt(val, 10), inputStereo)
            }}
            disabled={isLiveMode}
          >
            <SelectTrigger size="sm" className="flex-1 min-w-0 h-2" asChild>
              <Button variant="secondary" size="xs">
                <SelectValue className="text-xs" />
              </Button>
            </SelectTrigger>
            <SelectContent>
              <SelectItem className="text-xs" value="-1">
                No Input
              </SelectItem>
              {availableInputs.map((input) => (
                <SelectItem key={input.index} value={String(input.index)}>
                  {input.name}
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
          {inputChannel >= 0 && (
            <Button
              className={cn(
                'text-[10px] px-1',
                inputStereo && 'bg-purple-500/20 text-purple-500',
              )}
              variant="ghost"
              size="icon-xs"
              onClick={() => onSetInput(id, inputChannel, !inputStereo)}
              title={inputStereo ? 'Stereo input' : 'Mono input'}
              disabled={isLiveMode}
            >
              {inputStereo ? 'ST' : 'M'}
            </Button>
          )}
        </div>
        <div className="flex gap-1 items-center">
          <Input
            type="number"
            min={-80}
            max={12}
            step={0.5}
            value={Math.round(volume * 10) / 10}
            onChange={(e) => {
              const v = Math.max(-80, Math.min(12, Number(e.target.value)))
              onSetVolume(id, v)
            }}
            disabled={isLiveMode}
            className="h-5 w-min outline-none"
          />
          <span className="text-muted-foreground text-[10px]">dB</span>
        </div>
        <div
          className={cn(
            'absolute bottom-0 left-0 w-full h-1 cursor-row-resize transition-opacity',
          )}
          onPointerDown={(e) => e.stopPropagation()}
          onMouseDown={handleResizeMouseDown}
        />
      </div>
      <div
        ref={vuContainerRef}
        className="min-w-4 h-full bg-muted overflow-hidden relative shrink-0 group/vu cursor-ns-resize [grid-area:1/2/2/3]"
        onPointerDown={(e) => e.stopPropagation()}
        onMouseDown={handleVuVolumeMouseDown}
      >
        <div
          ref={vuMeterRef}
          className="absolute bottom-0 w-full pointer-events-none"
          style={{ height: '0%', transition: 'height 50ms ease-out' }}
        />
        {/* dB graduation marks */}
        {[12, 6, 0, -6, -12, -24, -48].map((db) => (
          <div
            key={db}
            className="absolute left-0 w-full pointer-events-none"
            style={{ bottom: `${((db + 80) / 92) * 100}%` }}
          >
            <div
              className={cn(
                'ml-auto h-px',
                db === 0 ? 'w-full bg-foreground/40' : 'w-1/4 bg-foreground/15',
              )}
            />
          </div>
        ))}
        <div
          className="absolute left-0 w-full h-px bg-white opacity-0 group-hover/vu:opacity-100 pointer-events-none transition-opacity"
          style={{ bottom: `${((volume + 80) / 92) * 100}%` }}
        />
      </div>
    </div>
  )
})

export default Track
