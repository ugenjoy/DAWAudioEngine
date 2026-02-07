import { RawData } from 'ws'

export function rawDataToString(rawData: RawData): string {
  if (typeof rawData === 'string') {
    return rawData
  } else if (rawData instanceof Buffer) {
    return rawData.toString('utf8')
  } else if (Array.isArray(rawData)) {
    // Buffer[]
    return Buffer.concat(rawData as Buffer[]).toString('utf8')
  } else if (rawData instanceof ArrayBuffer) {
    return Buffer.from(rawData).toString('utf8')
  } else {
    // Fallback for other types (objects) — stringify to preserve structure
    return JSON.stringify(rawData)
  }
}
