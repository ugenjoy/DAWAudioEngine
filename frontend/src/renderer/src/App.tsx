import { Route, Routes } from 'react-router'
import MainLayout from './layouts/MainLayout'

function App(): React.JSX.Element {
  return (
    <Routes>
      <Route path="/" element={<MainLayout />}></Route>
    </Routes>
  )
}

export default App
