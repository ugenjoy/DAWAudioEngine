import { useState } from 'react'
import { useNavigate } from 'react-router'
import { useProject } from '../contexts/project-provider'
import { useWebSocket } from '../contexts/websocket-provider'

export function useNavigateGuarded() {
  const navigate = useNavigate()
  const { playing } = useProject()
  const { send } = useWebSocket()
  const [pendingPath, setPendingPath] = useState<string | null>(null)

  function guardedNavigate(path: string) {
    if (playing) {
      setPendingPath(path)
    } else {
      navigate(path)
    }
  }

  function confirmStop() {
    send({ action: 'transport.stop' })
    if (pendingPath) navigate(pendingPath)
    setPendingPath(null)
  }

  function cancelStop() {
    setPendingPath(null)
  }

  return {
    guardedNavigate,
    confirmStop,
    cancelStop,
    showConfirm: pendingPath !== null,
  }
}
