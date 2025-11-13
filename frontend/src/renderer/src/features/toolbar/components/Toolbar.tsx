import { Button } from '@/shadcn/components/button'
import React from 'react'

function Toolbar(): React.JSX.Element {
  return (
    <div className="w-full p-2 flex justify-center gap-2 border-border border-b">
      <Button>Play</Button>
      <Button>Stop</Button>
    </div>
  )
}

export default Toolbar
