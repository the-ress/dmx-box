
export type ApiAuthMode =
  | 'open'
  | 'WEP'
  | 'WPA_PSK'
  | 'WPA_WPA2_PSK'
  | 'WPA2_PSK'
  | 'WPA2_WPA3_PSK'
  | 'WPA3_PSK'

export interface ApiModel {
  hostname: string
  ap: {
    ssid: string
    password: string
    auth_mode: ApiAuthMode
    channel: number
  }
  sta: {
    enabled: boolean
    ssid: string
    password: string
    auth_mode: ApiAuthMode
  }
}

export async function loadWiFiConfig(baseUrl: URL): Promise<ApiModel> {
  const response = await fetch(new URL('wifi-config', baseUrl))
  if (!response.ok) {
    throw await response.text()
  }
  const model = await response.json()
  return model
}

export async function submitWiFiConfig(baseUrl: URL, model: ApiModel): Promise<void> {
  const response = await fetch(new URL('wifi-config', baseUrl), {
    method: 'PUT',
    body: JSON.stringify(model)
  })
  if (!response.ok) {
    throw await response.text()
  }
}
