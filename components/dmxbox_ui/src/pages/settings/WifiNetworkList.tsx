import { JSX, useReducer } from 'react'
import WsSubscription from '../../components/WsSubscription'
import { useLastWsResponse } from '../../api'
import { ApFound, startApScan, stopApScan } from '../../api/wifi'
import { BsWifi, BsWifi1, BsWifi2 } from 'react-icons/bs'

function rssiToIcon(rssi: number): JSX.Element {
  if (rssi > -60) {
    return <BsWifi />
  } else if (rssi > -80) {
    return <BsWifi2 />
  } else {
    return <BsWifi1 />
  }
}

function networksReducer(networks: ApFound[], ap: ApFound): ApFound[] {
  const map = new Map(networks.map(n => [n.ssid, n]))
  const existing = map.get(ap.ssid)
  if (!existing || existing.rssi < ap.rssi) {
    map.set(ap.ssid, ap)
  }
  return [...map.values()].sort(n => n.rssi)
}

export default function WifiNetworkList() {
  const [networks, processAp] = useReducer(networksReducer, [])
  useLastWsResponse('settings/apFound', processAp)
  return (
    <WsSubscription start={startApScan} stop={stopApScan}>
      {...networks.map(net => (
        <div className="flex flex-row" key={net.ssid}>
          <div className="grow py-2">{net.ssid}</div>
          <div className="flex-none">{rssiToIcon(net.rssi)}</div>
        </div>
      ))}
    </WsSubscription>
  )
}
