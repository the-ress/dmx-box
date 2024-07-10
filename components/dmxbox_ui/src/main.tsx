import * as ReactDOM from 'react-dom/client'
import i18n from 'i18next'
import { initReactI18next } from 'react-i18next'
import { StrictMode } from 'react'
import en from './resources/en.json'
import { makeZodI18nMap } from 'zod-i18n-map'
import { z } from 'zod'
import App from './App'
import { createBrowserRouter, RouterProvider } from 'react-router-dom'
import { effectRoutes, MainPage, SettingsPage } from './pages'

const rootElement = document.getElementById('root')
if (!rootElement) {
  throw new Error('Failed to find the root element')
}

i18n.use(initReactI18next).init({
  fallbackLng: 'en',
  resources: { en },
})

z.setErrorMap(
  makeZodI18nMap({
    ns: ['zod', 'WiFi'],
    handlePath: { 'keyPrefix': '' }
  })
)

const router = createBrowserRouter([
  {
    path: '/',
    Component: App,
    children: [
      { path: '/', Component: MainPage },
      ...effectRoutes('effects'),
      { path: 'settings', Component: SettingsPage },
    ],
  },
])

const root = ReactDOM.createRoot(rootElement)
root.render(
  <StrictMode>
    <RouterProvider router={router} />
  </StrictMode>
)
