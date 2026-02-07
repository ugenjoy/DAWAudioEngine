import './index.css'
import { Route, Routes } from 'react-router'
import MainLayout from './shared/layouts/MainLayout'
import ProjectsHome from './pages/ProjectsHome'
import Sequencer from './features/sequencer/Sequencer'
import { WebSocketProvider } from './shared/contexts/websocket-provider'

function App() {
  return (
    <WebSocketProvider>
      <Routes>
        <Route element={<MainLayout />}>
          <Route index element={<ProjectsHome />} />
          <Route path="project/:projectId" element={<Sequencer />} />
        </Route>
      </Routes>
    </WebSocketProvider>
  )
}

export default App
