import { useCallback, useReducer, useRef, useState } from 'react'
import WsSubscription from '../../components/WsSubscription'
import { useApi, useLastWsResponse } from '../../api'
import { ApFound, putSta, startApScan, stopApScan } from '../../api/wifi'
import WifiNetwork from './WifiNetwork'
import WifiPasswordForm from './WifiPasswordForm'
import { WifiPasswordFields } from './schema'

function networksReducer(networks: ApFound[], ap: ApFound): ApFound[] {
  const map = new Map(networks.map(n => [n.ssid, n]))
  const existing = map.get(ap.ssid)
  if (!existing || existing.rssi < ap.rssi) {
    map.set(ap.ssid, ap)
  }
  return [...map.values()].sort(n => n.rssi)
}

export interface WifiNetworkListProps {
}

export default function WifiNetworkList({ }: WifiNetworkListProps) {
  const { apiUrl } = useApi()
  const [networks, processAp] = useReducer(networksReducer, [])
  const passwordDialog = useRef<HTMLDialogElement>()
  const [selectedNetwork, setSelectedNetwork] = useState<ApFound>()
  const joinSelectedNetwork = useCallback(({ password }: WifiPasswordFields) => {
    putSta(apiUrl, {
      enabled: true,
      ssid: selectedNetwork.ssid,
      authMode: selectedNetwork.authMode,
      password
    })
    passwordDialog.current?.close()
  }, [selectedNetwork])
  useLastWsResponse('settings/apFound', processAp)
  return (
    <WsSubscription start={startApScan} stop={stopApScan}>
      {...networks.map(net => <WifiNetwork
        key={net.ssid}
        onClick={() => { setSelectedNetwork(net), passwordDialog?.current.showModal() }}
        {...net}
      />)}
      <dialog ref={passwordDialog}>
        Password pls? {selectedNetwork?.ssid} SECURITY: {selectedNetwork?.authMode}
        <WifiPasswordForm
          onSubmit={joinSelectedNetwork}
        />
      </dialog>
    </WsSubscription>
  )
}
