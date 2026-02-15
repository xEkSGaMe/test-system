import React from 'react'

function App() {
  return (
    <div style={{ padding: '20px', fontFamily: 'Arial' }}>
      <h1>Test System - Web Client</h1>
      <p>Under Construction (TypeScript + React)</p>
      <p>Environment:</p>
      <ul>
        <li>API URL: {import.meta.env.VITE_API_URL || 'Not set'}</li>
        <li>Auth URL: {import.meta.env.VITE_AUTH_URL || 'Not set'}</li>
      </ul>
    </div>
  )
}

export default App