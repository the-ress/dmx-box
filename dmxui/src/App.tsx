import WiFiPage from "./pages/WiFi"
import { createApi, ApiContext } from "./api"

export interface AppProps {
  serverUrl: URL
}

export default function App({ serverUrl }: AppProps) {
  const api = createApi(serverUrl)
  return (
    <ApiContext.Provider value={api}>
      <WiFiPage />
    </ApiContext.Provider>
  )
}
