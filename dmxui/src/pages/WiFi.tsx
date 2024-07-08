import WiFiForm from './WiFi/WiFiForm'
import { wrapPromise } from '../helpers/wrapPromise.ts'
import { Suspense, useContext, useEffect, useState } from 'react'
import { startApScan, stopApScan } from '../api/wifi'
import { apiModelFromFields, apiModelToFields } from './WiFi/api.ts'
import { WiFiFields } from './WiFi/schema.ts'
import { ApiContext, ApiWsResponse } from '../api/index.ts'

function processWsResponse<Type extends ApiWsResponse['type']>(
  type: Type,
  effect: (response: Extract<ApiWsResponse, { type: Type }>) => void
): void {
  const { lastWsResponse } = useContext(ApiContext)
  useEffect(() => {
    if (lastWsResponse && lastWsResponse.type === type) {
      effect(lastWsResponse as Extract<ApiWsResponse, { type: Type }>)
    }
  }, [lastWsResponse])
}

export default function WiFiPage() {
  const { sendWsRequest, fetchSettings, submitSettings } = useContext(ApiContext)
  const load = wrapPromise(fetchSettings().then(apiModelToFields))
  const submit = async (fields: WiFiFields) => {
    const model = apiModelFromFields(fields);
    await submitSettings(model);
  }
  const [accessPoints, setAccessPoints] = useState<string[]>([])
  processWsResponse('settings/apFound', apFound => {
    setAccessPoints((prev) => prev.concat(apFound.ssid))
  })
  useEffect(() => {
    sendWsRequest(startApScan())
    return () => sendWsRequest(stopApScan())
  }, [sendWsRequest])
  return (
    <div>
      <Suspense fallback={<div>Loading</div>}>
        <WiFiForm onLoad={load} onSubmit={submit} />
        {...accessPoints.map(ap => <div key={ap}>{ap}</div>)}
      </Suspense>
    </div>
  )
}
