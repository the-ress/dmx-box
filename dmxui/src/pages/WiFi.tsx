import WiFiForm from './WiFi/WiFiForm'
import { wrapPromise } from '../helpers/wrapPromise.ts'
import { Suspense, useEffect, useState } from 'react'
import { loadWiFiConfig, submitWiFiConfig } from '../api/wifi'
import { apiModelFromFields, apiModelToFields } from './WiFi/api.ts'
import { WiFiFields } from './WiFi/schema.ts'
import { SendJsonMessage, LastMessage } from 'react-use-websocket'

export interface WiFiPageProps {
  apiUrl: URL
  sendJsonMessage: SendJsonMessage
  lastMessage: LastMessage
}

export default function WiFiPage({ apiUrl, lastMessage, sendJsonMessage }: WiFiPageProps) {
  const load = wrapPromise(loadWiFiConfig(apiUrl).then(apiModelToFields))
  const submit = async (fields: WiFiFields) => {
    const model = apiModelFromFields(fields);
    await submitWiFiConfig(apiUrl, model);
  }
  const [accessPoints, setAccessPoints] = useState<string[]>([])
  useEffect(() => {
    sendJsonMessage({ type: 'START_AP_SCAN' })
    return () => sendJsonMessage({ type: 'STOP_AP_SCAN' })
  }, [sendJsonMessage])
  useEffect(() => {
    if (lastMessage) {
      setAccessPoints((prev) => prev.concat(lastMessage.data))
    }
  }, [lastMessage])
  return (
    <div>
      <Suspense fallback={<div>Loading</div>}>
        <WiFiForm onLoad={load} onSubmit={submit} />
        {...accessPoints.map(ap => <div key={ap}>{ap}</div>)}
      </Suspense>
    </div>
  )
}
