import React from 'react'
import { Play, Square } from 'lucide-react'
import { Button } from '@/shared/shadcn/components/button'

function Toolbar(): React.JSX.Element {
  return (
    <div className="w-full p-2 flex flex-col gap-2 border-border border-b">
      <Button variant="ghost" size="icon-sm">
        <Play />
      </Button>
      <Button variant="ghost" size="icon-sm">
        <Square />
      </Button>
    </div>
  )
}

export default Toolbar
