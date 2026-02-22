import './index.css'
import { Route, Routes } from 'react-router'
import HomePage from './pages/HomePage'
import { WebSocketProvider } from './shared/contexts/websocket-provider'
import { ProjectsProvider } from './shared/contexts/projects-provider'
import { ProjectProvider } from './shared/contexts/project-provider'
import { AudioDevicesProvider } from './shared/contexts/audio-devices-provider'
import { ModeProvider } from './shared/contexts/mode-provider'

function App() {
  return (
    <WebSocketProvider>
      <AudioDevicesProvider>
        <ProjectsProvider>
          <ProjectProvider>
            <ModeProvider>
              <Routes>
                <Route index element={<HomePage />} />
              </Routes>
            </ModeProvider>
          </ProjectProvider>
        </ProjectsProvider>
      </AudioDevicesProvider>
    </WebSocketProvider>
  )
}

export default App
