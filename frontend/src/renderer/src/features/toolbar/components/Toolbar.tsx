import { Button } from '@/shadcn/components/button'
import React, { useState } from 'react'

function Toolbar(): React.JSX.Element {
  const [testStatus, setTestStatus] = useState<string>('')
  const [isTesting, setIsTesting] = useState(false)

  const handleTestClick = async (): Promise<void> => {
    setIsTesting(true)
    setTestStatus('Testing backend connection...')

    try {
      const result = await globalThis.api.getHealth()

      if (result) {
        setTestStatus(`✓ ${result}`)
      } else {
        setTestStatus(`✗ ${result}`)
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error'
      setTestStatus(`✗ Connection failed: ${errorMessage}`)
    } finally {
      setIsTesting(false)

      // Clear status after 5 seconds
      setTimeout(() => {
        setTestStatus('')
      }, 5000)
    }
  }

  return (
    <div className="w-full p-2 flex flex-col gap-2 border-border border-b">
      <div className="flex justify-center gap-2">
        <Button>Play</Button>
        <Button>Stop</Button>
        <Button onClick={handleTestClick} variant="outline" disabled={isTesting}>
          {isTesting ? 'Testing...' : 'Test'}
        </Button>
      </div>
      {testStatus && (
        <div className="text-sm text-center text-muted-foreground px-2">{testStatus}</div>
      )}
    </div>
  )
}

export default Toolbar
