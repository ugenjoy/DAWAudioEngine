import Toolbar from '../features/toolbar/components/Toolbar'
import React from 'react'
import { Outlet } from 'react-router'

function MainLayout(): React.JSX.Element {
  return (
    <main className="flex flex-col h-screen">
      <Toolbar />
      <div className="flex-1 overflow-auto">
        <Outlet />
      </div>
    </main>
  )
}

export default MainLayout
