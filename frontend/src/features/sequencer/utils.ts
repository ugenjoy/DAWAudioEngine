let cachedStyles: CSSStyleDeclaration | null = null

export function getCSSVar(value: string): string {
  if (!cachedStyles) {
    const element = document.getElementById('root')
    if (!element) return ''
    cachedStyles = globalThis.getComputedStyle(element)
  }
  return cachedStyles.getPropertyValue(value)
}
