import { useState } from 'react'
import { useAudioDevices } from '@/shared/contexts/audio-devices-provider'
import { Button } from '@/shared/shadcn/components/button'
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
} from '@/shared/shadcn/components/dialog'
import { IconSettings } from '@tabler/icons-react'
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from '@/shared/shadcn/components/select'
import { Label } from '@/shared/shadcn/components/label'

export function AudioSettingsDialog() {
  const { deviceTypes, currentDevice, setAudioDevice, fetchDevices } =
    useAudioDevices()
  const [open, setOpen] = useState(false)

  const currentType = deviceTypes.find(
    (dt) => dt.name === currentDevice?.deviceType,
  )

  function handleDeviceTypeChange(typeName: string) {
    const type = deviceTypes.find((dt) => dt.name === typeName)
    if (!type) return
    const output = type.outputDevices[0] ?? ''
    const input = type.inputDevices[0] ?? ''
    setAudioDevice(typeName, output, input)
  }

  function handleInputChange(inputDevice: string) {
    if (!currentDevice) return
    setAudioDevice(
      currentDevice.deviceType,
      currentDevice.outputDevice,
      inputDevice,
    )
  }

  function handleOutputChange(outputDevice: string) {
    if (!currentDevice) return
    setAudioDevice(
      currentDevice.deviceType,
      outputDevice,
      currentDevice.inputDevice,
    )
  }

  function handleSampleRateChange(value: string) {
    if (!currentDevice) return
    setAudioDevice(
      currentDevice.deviceType,
      currentDevice.outputDevice,
      currentDevice.inputDevice,
      Number(value),
    )
  }

  function handleBufferSizeChange(value: string) {
    if (!currentDevice) return
    setAudioDevice(
      currentDevice.deviceType,
      currentDevice.outputDevice,
      currentDevice.inputDevice,
      undefined,
      Number(value),
    )
  }

  return (
    <Dialog
      open={open}
      onOpenChange={(v) => {
        setOpen(v)
        if (v) fetchDevices()
      }}
    >
      <DialogTrigger asChild>
        <Button variant="ghost" size="icon-sm" title="Audio settings">
          <IconSettings size={16} />
        </Button>
      </DialogTrigger>
      <DialogContent>
        <DialogHeader>
          <DialogTitle>Audio Settings</DialogTitle>
        </DialogHeader>
        <div className="flex flex-col gap-4">
          {deviceTypes.length > 1 && (
            <div className="flex flex-col gap-1">
              <Label className="text-xs font-medium">Driver</Label>
              <Select
                onValueChange={handleDeviceTypeChange}
                value={currentDevice?.deviceType}
              >
                <SelectTrigger className="w-full">
                  <SelectValue placeholder="Driver" />
                </SelectTrigger>
                <SelectContent>
                  {deviceTypes.map((dt) => (
                    <SelectItem key={dt.name} value={dt.name}>
                      {dt.name}
                    </SelectItem>
                  ))}
                </SelectContent>
              </Select>
            </div>
          )}

          <div className="flex flex-row justify-between">
            {currentDevice?.availableSampleRates?.length &&
              currentDevice.availableSampleRates.length > 0 && (
                <div className="flex flex-col gap-1">
                  <Label className="text-xs font-medium">Sample Rate</Label>
                  <Select
                    onValueChange={handleSampleRateChange}
                    value={String(currentDevice.sampleRate)}
                  >
                    <SelectTrigger>
                      <SelectValue placeholder="Sample Rate" />
                    </SelectTrigger>
                    <SelectContent>
                      {currentDevice.availableSampleRates.map((rate) => (
                        <SelectItem key={rate} value={String(rate)}>
                          {rate >= 1000 ? `${rate / 1000} kHz` : `${rate} Hz`}
                        </SelectItem>
                      ))}
                    </SelectContent>
                  </Select>
                </div>
              )}

            {currentDevice?.availableBufferSizes?.length &&
              currentDevice.availableBufferSizes.length > 0 && (
                <div className="flex flex-col gap-1">
                  <Label className="text-xs font-medium">Buffer Size</Label>
                  <Select
                    onValueChange={handleBufferSizeChange}
                    value={String(currentDevice.bufferSize)}
                  >
                    <SelectTrigger>
                      <SelectValue placeholder="Buffer Size" />
                    </SelectTrigger>
                    <SelectContent>
                      {currentDevice.availableBufferSizes.map((size) => (
                        <SelectItem key={size} value={String(size)}>
                          {size} samples
                        </SelectItem>
                      ))}
                    </SelectContent>
                  </Select>
                </div>
              )}
          </div>

          <div className="flex flex-col gap-1">
            <Label className="text-xs font-medium">Output Device</Label>
            <Select
              onValueChange={handleOutputChange}
              value={currentDevice?.outputDevice}
            >
              <SelectTrigger className="w-full">
                <SelectValue placeholder="Output Device" />
              </SelectTrigger>
              <SelectContent>
                {currentType?.outputDevices.map((name) => (
                  <SelectItem key={name} value={name}>
                    {name}
                  </SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>

          <div className="flex flex-col gap-1">
            <Label className="text-xs font-medium">Input Device</Label>
            <Select
              onValueChange={handleInputChange}
              value={currentDevice?.inputDevice}
            >
              <SelectTrigger>
                <SelectValue placeholder="Input Device" className="w-full" />
              </SelectTrigger>
              <SelectContent>
                {currentType?.inputDevices.map((name) => (
                  <SelectItem key={name} value={name}>
                    {name}
                  </SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>
        </div>
      </DialogContent>
    </Dialog>
  )
}
