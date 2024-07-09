import WiFiForm from './WiFi/WiFiForm'
import { ReactNode, useEffect, useState } from 'react'
import { ApFound, getWiFiConfig, putWiFiConfig, startApScan, stopApScan } from '../api/wifi'
import { apiModelFromFields, apiModelToFields } from './WiFi/api.ts'
import { WiFiFields } from './WiFi/schema.ts'
import { WsRequest, useLastWsResponse, useApi, useWs } from '../api'

export interface WsSubscriptionProps {
  start: WsRequest
  stop: WsRequest
  children: ReactNode
}

function WsSubscription({ children, start, stop }: WsSubscriptionProps) {
  const { sendJsonMessage } = useWs()
  useEffect(() => {
    sendJsonMessage(start)
    return () => sendJsonMessage(stop)
  }, [start, stop])
  return children;
}

export default function WiFiPage() {
  const { apiUrl } = useApi()

  const [accessPoints, setAccessPoints] = useState<ApFound[]>([])
  const [fields, setFields] = useState(undefined)

  const submit = async (fields: WiFiFields) => {
    const model = apiModelFromFields(fields);
    await putWiFiConfig(apiUrl, model);
  }

  useEffect(() => {
    getWiFiConfig(apiUrl).then(apiModelToFields).then(setFields)
  }, [apiUrl])

  useLastWsResponse(
    'settings/apFound',
    ap => setAccessPoints(prev => prev.concat(ap))
  )

  return (
    <WsSubscription start={startApScan} stop={stopApScan}>
      {fields
        ? <WiFiForm fields={fields} onSubmit={submit} />
        : <div>Loading</div>
      }
      {...accessPoints.map(ap => <div key={ap.mac}>{ap.ssid}</div>)}
    </WsSubscription>
  )
}

