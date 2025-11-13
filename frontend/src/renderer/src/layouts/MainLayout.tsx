import Toolbar from '../features/toolbar/components/Toolbar'
import React from 'react'
import { Outlet } from 'react-router'

function MainLayout(): React.JSX.Element {
  return (
    <main>
      <Toolbar />
      <Outlet />
    </main>
  )
}

export default MainLayout
