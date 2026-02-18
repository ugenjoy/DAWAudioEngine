import { getCSSVar } from '../utils'

export function drawClip(
  rect: { x: number; y: number; width: number; height: number },
  ctx: CanvasRenderingContext2D,
) {
  ctx.fillStyle = getCSSVar('--track-fill')
  ctx.fillRect(rect.x, rect.y, rect.width, rect.height)

  ctx.strokeStyle = getCSSVar('--track-stroke')
  ctx.strokeRect(rect.x, rect.y, rect.width, rect.height)
}
