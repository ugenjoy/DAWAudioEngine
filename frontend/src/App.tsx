import './index.css'
import { Route, Routes, Navigate, useLocation } from 'react-router'
import { IconLoader } from '@tabler/icons-react'
import ProjectHomePage from './pages/ProjectHomePage'
import EditPage from './pages/EditPage'
import LivePage from './pages/LivePage'
import EventsPage from './pages/EventsPage'
import MarkersPage from './pages/MarkersPage'
import { WebSocketProvider } from './shared/contexts/websocket-provider'
import { useWebSocket } from './shared/contexts/websocket-provider'
import { ProjectsProvider } from './shared/contexts/projects-provider'
import { ProjectProvider } from './shared/contexts/project-provider'
import { AudioDevicesProvider } from './shared/contexts/audio-devices-provider'
import { ModeProvider } from './shared/contexts/mode-provider'
import { EventsProvider } from './shared/contexts/events-provider'
import { SetlistProvider } from './shared/contexts/setlist-provider'
import { MarkersProvider } from './shared/contexts/markers-provider'
import ConnectionPage from './pages/ConnectionPage'

function RequireConnection({ children }: { children: React.ReactNode }) {
  const { isConnected, autoConnect } = useWebSocket()
  const location = useLocation()

  if (!isConnected) {
    if (autoConnect) {
      return (
        <main className="flex h-screen items-center justify-center gap-4">
          <IconLoader className="animate-spin size-8" />
        </main>
      )
    }
    return <Navigate to="/connect" state={{ from: location }} replace />
  }

  return <>{children}</>
}

function App() {
  return (
    <WebSocketProvider>
      <AudioDevicesProvider>
        <ProjectsProvider>
          <ProjectProvider>
            <ModeProvider>
              <EventsProvider>
                <MarkersProvider>
                  <SetlistProvider>
                    <Routes>
                      <Route path="connect" element={<ConnectionPage />} />
                      <Route
                        path="*"
                        element={
                          <RequireConnection>
                            <Routes>
                              <Route index element={<ProjectHomePage />} />
                              <Route path="edit/:songId" element={<EditPage />} />
                              <Route path="live" element={<LivePage />} />
                              <Route path="events" element={<EventsPage />} />
                              <Route path="markers" element={<MarkersPage />} />
                            </Routes>
                          </RequireConnection>
                        }
                      />
                    </Routes>
                  </SetlistProvider>
                </MarkersProvider>
              </EventsProvider>
            </ModeProvider>
          </ProjectProvider>
        </ProjectsProvider>
      </AudioDevicesProvider>
    </WebSocketProvider>
  )
}

export default App
