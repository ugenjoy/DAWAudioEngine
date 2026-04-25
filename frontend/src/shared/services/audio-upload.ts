import type { Clip } from '../models/clip'

function getBackendBaseUrl(): string {
  if (import.meta.env.VITE_WS_AUTO_CONNECT === 'true') {
    const protocol =
      globalThis.location.protocol === 'https:' ? 'https:' : 'http:'
    return `${protocol}//${globalThis.location.host}`
  }
  return 'http://localhost:8080'
}

export async function uploadAudioClip(
  file: File,
  trackId: string,
  position: number,
): Promise<Clip> {
  const formData = new FormData()
  formData.append('file', file)
  formData.append('trackId', trackId)
  formData.append('position', String(position))

  const res = await fetch(`${getBackendBaseUrl()}/api/audio/upload`, {
    method: 'POST',
    body: formData,
  })

  if (!res.ok) {
    const err = await res.json().catch(() => ({ error: 'Upload failed' }))
    throw new Error(err.error ?? 'Upload failed')
  }

  return res.json()
}
