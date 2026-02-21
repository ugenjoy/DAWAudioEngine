import { Button } from '@/shared/shadcn/components/button'
import { Slider } from '@/shared/shadcn/components/slider'
import { cn } from '@/shared/shadcn/lib/utils'
import { getCSSVar } from '../utils'

type TrackProps = {
  name: string
  mute: boolean
  solo: boolean
  volume: number
  color: string
  height: number
}

function Track({
  name,
  mute,
  solo,
  volume,
  color,
  height,
}: Readonly<TrackProps>) {
  function setMute(v: boolean) {
    console.log(v)
  }

  function setSolo(v: boolean) {
    console.log(v)
  }

  function setVolume(v: number) {
    console.log(v)
  }

  return (
    <div
      className="p-2 pb-4 border-b border-border w-full text-xs flex flex-col gap-2"
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
        <div>
          <Button
            className={cn(
              'font-bold',
              mute && 'bg-amber-500/20 text-amber-500',
            )}
            variant="ghost"
            size="icon-xs"
            onClick={() => setMute(!mute)}
          >
            M
          </Button>
          <Button
            className={cn('font-bold', solo && 'bg-blue-500/20 text-blue-500')}
            variant="ghost"
            size="icon-xs"
            onClick={() => setSolo(!solo)}
          >
            S
          </Button>
        </div>
      </div>
      <div className="p-1 flex gap-1">
        <Slider
          min={0}
          max={1}
          step={0.01}
          defaultValue={[volume]}
          onValueChange={(value) => setVolume(value[0])}
          className="flex-1 h-1 bg-muted rounded-full cursor-pointer"
        />
      </div>
    </div>
  )
}

export default Track
