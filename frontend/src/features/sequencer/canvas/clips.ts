import { getCSSVar } from '../utils'

type ClipRect = {
  x: number
  y: number
  width: number
  height: number
}

export function drawClip(
  rect: ClipRect,
  fillColor: string,
  strokeColor: string,
  waveform: number[] | undefined,
  ctx: CanvasRenderingContext2D,
) {
  ctx.fillStyle = getCSSVar(fillColor)
  ctx.fillRect(rect.x, rect.y, rect.width, rect.height)

  if (waveform && waveform.length >= 2) {
    drawWaveform(rect, strokeColor, waveform, ctx)
  }

  ctx.strokeStyle = getCSSVar(strokeColor)
  ctx.strokeRect(rect.x, rect.y, rect.width, rect.height)
}

function drawWaveform(
  rect: ClipRect,
  color: string,
  waveform: number[],
  ctx: CanvasRenderingContext2D,
) {
  const sourcePoints = waveform.length / 2

  // Limit drawn points to available pixels (1 point per pixel max)
  const maxDrawnPoints = Math.max(1, Math.ceil(rect.width))
  const drawnPoints = Math.min(sourcePoints, maxDrawnPoints)

  if (drawnPoints <= 0) return

  const centerY = rect.y + rect.height / 2
  const halfHeight = rect.height / 2
  const pixelsPerPoint = rect.width / drawnPoints
  // How many source points map to one drawn point
  const step = sourcePoints / drawnPoints

  ctx.fillStyle = getCSSVar(color)
  ctx.beginPath()

  // Top half (max values) — left to right
  for (let i = 0; i < drawnPoints; i++) {
    const range = getMinMaxForRange(waveform, i * step, (i + 1) * step)
    const x = rect.x + i * pixelsPerPoint
    const y = centerY - range.max * halfHeight

    if (i === 0) {
      ctx.moveTo(x, y)
    } else {
      ctx.lineTo(x, y)
    }
  }

  // Bottom half (min values) — right to left
  for (let i = drawnPoints - 1; i >= 0; i--) {
    const { min } = getMinMaxForRange(waveform, i * step, (i + 1) * step)
    const x = rect.x + i * pixelsPerPoint
    const y = centerY - min * halfHeight

    ctx.lineTo(x, y)
  }

  ctx.closePath()
  ctx.fill()
}

/** Aggregate min/max across a range of source points */
function getMinMaxForRange(
  waveform: number[],
  startPoint: number,
  endPoint: number,
): { min: number; max: number } {
  const first = Math.floor(startPoint)
  const last = Math.min(Math.ceil(endPoint), waveform.length / 2)

  let min = 1.0
  let max = -1.0

  for (let i = first; i < last; i++) {
    const pMin = waveform[i * 2]
    const pMax = waveform[i * 2 + 1]
    if (pMin < min) min = pMin
    if (pMax > max) max = pMax
  }

  return { min, max }
}
