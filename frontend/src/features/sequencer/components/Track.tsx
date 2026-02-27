import { Button } from '@/shared/shadcn/components/button'
import { Slider } from '@/shared/shadcn/components/slider'
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
  onSetInput: (trackId: string, inputChannel: number, stereo: boolean) => void
  onSetMonitoring: (trackId: string, monitoring: boolean) => void
  onSetMute: (trackId: string, mute: boolean) => void
  onSetSolo: (trackId: string, solo: boolean) => void
  onSetVolume: (trackId: string, volume: number) => void
}

function Track({
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
  onSetInput,
  onSetMonitoring,
  onSetMute,
  onSetSolo,
  onSetVolume,
}: Readonly<TrackProps>) {
  const { isLiveMode } = useMode()

  return (
    <div
      className="p-2 pb-4 bg-foreground/5 border-b border-border w-full text-xs flex flex-col gap-2"
      style={{
        height: height + 'px',
      }}
    >
      <div className="flex flex-row items-center w-full">
        <div className="flex flex-row gap-2 items-center">
          <div
            className="rounded-full size-2"
            style={{
              backgroundColor: getCSSVar(color),
            }}
          />
          {name}
        </div>
        <div className="flex-1" />
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
            title={isLiveMode ? 'Not available in Live mode' : 'Input monitoring'}
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
            className={cn('font-bold', solo && 'bg-blue-500/20 text-blue-500')}
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
          <SelectTrigger size="sm" className="flex-1 min-w-0">
            <SelectValue className="text-xs" />
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
      <div className="p-1 flex gap-1">
        <Slider
          min={0}
          max={1}
          step={0.01}
          value={[volume]}
          onValueChange={(value) => onSetVolume(id, value[0])}
          className="flex-1 h-1 bg-muted rounded-full cursor-pointer"
          disabled={isLiveMode}
        />
      </div>
    </div>
  )
}

export default Track
