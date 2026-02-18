export function getCSSVar(value: string): string {
  const element = document.getElementById('root')
  if (!element) return ''

  const styles = globalThis.getComputedStyle(element)
  return styles.getPropertyValue(value)
}
