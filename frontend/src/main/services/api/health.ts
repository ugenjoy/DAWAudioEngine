import axios from 'axios'

export default async function getHealth(): Promise<string> {
  try {
    const { data } = await axios.get('http://localhost:8080/health', {
      timeout: 5000
    })
    return `Backend OK - Response: "${data}"`
  } catch (error) {
    if (axios.isAxiosError(error)) {
      if (error.code === 'ECONNREFUSED') {
        throw new Error('Backend not running on port 8080')
      }
      if (error.code === 'ETIMEDOUT') {
        throw new Error('Backend timeout - took too long to respond')
      }
      throw new Error(`Backend error: ${error.message}`)
    }
    throw new Error('Unknown error occurred')
  }
}
