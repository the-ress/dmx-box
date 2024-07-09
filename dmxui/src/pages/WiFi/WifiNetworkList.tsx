import { useReducer } from 'react'
import WsSubscription from '../../components/WsSubscription'
import { useLastWsResponse } from '../../api'
import { ApFound, startApScan, stopApScan } from '../../api/wifi'

interface WiFiNetwork {
  name: string
  signalStrength: number
}

function networksReducer(networks: WiFiNetwork[], ap: ApFound): WiFiNetwork[] {
  const map = new Map(networks.map(n => [n.name, n]))
  const existing = map.get(ap.ssid)
  if (existing) {
    if (ap.rssi > existing.signalStrength) {
      existing.signalStrength = ap.rssi
    }
  } else {
    map.set(ap.ssid, { name: ap.ssid, signalStrength: ap.rssi })
  }
  return [...map.values()].sort(n => n.signalStrength)
}

export default function WifiNetworkList() {
  const [networks, processAp] = useReducer(networksReducer, [])
  useLastWsResponse('settings/apFound', processAp)
  return (
    <WsSubscription start={startApScan} stop={stopApScan}>
      {...networks.map(net => (
        <div key={net.name}>{net.name} ({net.signalStrength} dBm)</div>
      ))}
    </WsSubscription>
  )
}
