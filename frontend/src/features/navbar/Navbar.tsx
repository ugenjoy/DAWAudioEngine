import { Button } from '@/shared/shadcn/components/button'
import React from 'react'

function Navbar(): React.JSX.Element {
  return (
    <div className="w-full p-2 flex border-border border-b">
      <Button variant="ghost" size="sm">
        File
      </Button>
      <Button variant="ghost" size="sm">
        Edit
      </Button>
    </div>
  )
}

export default Navbar
