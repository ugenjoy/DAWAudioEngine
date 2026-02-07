import ReactDOM from 'react-dom/client'
import App from './App'
import { BrowserRouter } from 'react-router'
import { ProjectProvider } from './shared/contexts/project-provider'

ReactDOM.createRoot(document.getElementById('root') as HTMLElement).render(
  <BrowserRouter>
    <ProjectProvider>
      <App />
    </ProjectProvider>
  </BrowserRouter>,
)
