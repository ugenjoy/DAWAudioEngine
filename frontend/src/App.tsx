import './index.css'
import { Route, Routes } from 'react-router'
import HomePage from './pages/HomePage'
import { WebSocketProvider } from './shared/contexts/websocket-provider'
import { ProjectsProvider } from './shared/contexts/projects-provider'
import { ProjectProvider } from './shared/contexts/project-provider'
import { AudioDevicesProvider } from './shared/contexts/audio-devices-provider'
import { ModeProvider } from './shared/contexts/mode-provider'
import { EventsProvider } from './shared/contexts/events-provider'
import ConnectionPage from './pages/ConnectionPage'

function App() {
  return (
    <WebSocketProvider>
      <AudioDevicesProvider>
        <ProjectsProvider>
          <ProjectProvider>
            <ModeProvider>
              <EventsProvider>
                <Routes>
                  <Route index element={<HomePage />} />
                  <Route path="connect" element={<ConnectionPage />} />
                </Routes>
              </EventsProvider>
            </ModeProvider>
          </ProjectProvider>
        </ProjectsProvider>
      </AudioDevicesProvider>
    </WebSocketProvider>
  )
}

export default App
