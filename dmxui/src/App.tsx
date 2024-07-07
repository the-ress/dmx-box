import useWebSocket from "react-use-websocket";
import WiFiPage from "./pages/WiFi";

const apiUrl = new URL('/api/',
  window.location.hash ? window.location.hash.substring(1) : window.origin
)
const wsUrl = new URL('ws', apiUrl)
wsUrl.protocol = wsUrl.protocol.replace(/^http/, 'ws')

export default function App() {
  const { sendJsonMessage, lastMessage } = useWebSocket(wsUrl)
  return <WiFiPage
    sendJsonMessage={sendJsonMessage}
    lastMessage={lastMessage}
    apiUrl={apiUrl}
  />
}
