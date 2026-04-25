import { createContext, useContext } from 'react'
import { useSession } from './session-provider'

type AppMode = 'live' | 'edit'

type ModeProviderState = {
  mode: AppMode
  isLiveMode: boolean
  isEditMode: boolean
}

const ModeProviderContext = createContext<ModeProviderState>({
  mode: 'live',
  isLiveMode: true,
  isEditMode: false,
})

export function ModeProvider({ children }: Readonly<{ children: React.ReactNode }>) {
  const { session } = useSession()

  const mode: AppMode = session.activity === 'editing' ? 'edit' : 'live'

  const value: ModeProviderState = {
    mode,
    isLiveMode: session.activity === 'performing',
    isEditMode: session.activity === 'editing',
  }

  return (
    <ModeProviderContext.Provider value={value}>
      {children}
    </ModeProviderContext.Provider>
  )
}

export const useMode = () => {
  const context = useContext(ModeProviderContext)
  if (context === undefined)
    throw new Error('useMode must be used within a ModeProvider')
  return context
}
