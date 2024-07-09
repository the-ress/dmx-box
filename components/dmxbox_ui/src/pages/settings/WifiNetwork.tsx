import { JSX } from "react"
import { BsWifi, BsWifi1, BsWifi2 } from "react-icons/bs"

export interface WifiNetworkProps {
  ssid: string
  rssi: number
  onClick?: () => void
}

function rssiToIcon(rssi: number): JSX.Element {
  if (rssi > -60) {
    return <BsWifi />
  } else if (rssi > -80) {
    return <BsWifi2 />
  } else {
    return <BsWifi1 />
  }
}

export default function WifiNetwork({ onClick, ssid, rssi }: WifiNetworkProps) {
  return (
    <button type="button" disabled={!onClick} onClick={onClick} className="flex flex-row">
      <div className="grow py-2">{ssid}</div>
      <div className="flex-none">{rssiToIcon(rssi)}</div>
    </button>
  )

}
