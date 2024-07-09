import WiFiForm from './WiFi/WiFiForm'
import { useEffect, useState } from 'react'
import { getWiFiConfig, putWiFiConfig } from '../api/wifi'
import { apiModelFromFields, apiModelToFields } from './WiFi/api.ts'
import { WiFiFields } from './WiFi/schema.ts'
import { useApi } from '../api'

export default function WiFiPage() {
  const { apiUrl } = useApi()

  const [fields, setFields] = useState(undefined)

  const submit = async (fields: WiFiFields) => {
    const model = apiModelFromFields(fields);
    await putWiFiConfig(apiUrl, model);
  }

  useEffect(() => {
    getWiFiConfig(apiUrl).then(apiModelToFields).then(setFields)
  }, [apiUrl])

  return fields
    ? <WiFiForm fields={fields} onSubmit={submit} />
    : <div>Loading</div>
}

