import WiFiForm from './WiFiForm'
import { useEffect, useState } from 'react'
import { getWiFiConfig, putWiFiConfig } from '../../api/wifi'
import { apiModelFromFields, apiModelToFields } from './api'
import { WiFiFields } from './schema'
import { useApi } from '../../api'

export default function SettingsPage() {
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

