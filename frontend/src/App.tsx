import './index.css'
import { Route, Routes } from 'react-router'
import HomePage from './pages/HomePage'
import { WebSocketProvider } from './shared/contexts/websocket-provider'
import { ProjectsProvider } from './shared/contexts/projects-provider'
import { ProjectProvider } from './shared/contexts/project-provider'

function App() {
  return (
    <WebSocketProvider>
      <ProjectsProvider>
        <ProjectProvider>
          <Routes>
            <Route index element={<HomePage />} />
          </Routes>
        </ProjectProvider>
      </ProjectsProvider>
    </WebSocketProvider>
  )
}

export default App
