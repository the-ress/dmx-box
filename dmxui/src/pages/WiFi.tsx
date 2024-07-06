import WiFiForm from './WiFi/WiFiForm'
import { wrapPromise } from '../helpers/wrapPromise.ts'
import { Suspense } from 'react'
import { loadWiFiConfig, submitWiFiConfig } from './WiFi/api.ts'

export default function WiFiPage() {
  const loader = wrapPromise(loadWiFiConfig())
  return (
    <Suspense fallback={<div>Loading</div>}>
      <WiFiForm onLoad={loader} onSubmit={submitWiFiConfig} />
    </Suspense>
  )
}
