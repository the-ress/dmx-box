import WiFiForm from './WiFi/WiFiForm'
import { ReactNode, useEffect, useState } from 'react'
import { ApFound, getWiFiConfig, putWiFiConfig, startApScan, stopApScan } from '../api/wifi'
import { apiModelFromFields, apiModelToFields } from './WiFi/api.ts'
import { WiFiFields } from './WiFi/schema.ts'
import { ApiWsRequest, ApiWsResponse, useDmxboxApi, useDmxboxWebSocket } from '../api/index.ts'

function useLastWsResponse<Type extends ApiWsResponse['type']>(
  type: Type,
  effect: (response: Extract<ApiWsResponse, { type: Type }>) => void
): void {
  const { lastJsonMessage } = useDmxboxWebSocket()
  useEffect(() => {
    if (lastJsonMessage?.type === type) {
      effect(lastJsonMessage as Extract<ApiWsResponse, { type: Type }>)
    }
  }, [lastJsonMessage])
}

export interface DmxboxWsSubscriptionProps {
  start: ApiWsRequest
  stop: ApiWsRequest
  children: ReactNode
}

function DmxboxWsSubscription({ children, start, stop }: DmxboxWsSubscriptionProps) {
  const { sendJsonMessage } = useDmxboxWebSocket()
  useEffect(() => {
    sendJsonMessage(start)
    return () => sendJsonMessage(stop)
  }, [start, stop])
  return children;
}

export default function WiFiPage() {
  const { apiUrl } = useDmxboxApi()

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
    <DmxboxWsSubscription start={startApScan} stop={stopApScan}>
      {fields
        ? <WiFiForm fields={fields} onSubmit={submit} />
        : <div>Loading</div>
      }
      {...accessPoints.map(ap => <div key={ap.mac}>{ap.ssid}</div>)}
    </DmxboxWsSubscription>
  )
}

