function put<T>(endpoint: string) {
  return async (baseUrl: URL, model: T) => {
    const response = await fetch(new URL(endpoint, baseUrl), {
      method: 'PUT',
      body: JSON.stringify(model)
    })
    if (!response.ok) {
      throw await response.text()
    }
  }
}

function get<T>(endpoint: string) {
  return async (baseUrl: URL): Promise<T> => {
    const response = await fetch(new URL(endpoint, baseUrl))
    if (!response.ok) {
      throw await response.text()
    }
    const model = await response.json()
    return model
  }
}


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

export interface PutStaModel {
  enabled: boolean
  ssid: string
  authMode: ApiAuthMode
  password: string
}

export const getWiFiConfig = get<ApiModel>('wifi-config')

export const putWiFiConfig = put<ApiModel>('wifi-config');
export const putSta = put<PutStaModel>('settings/sta');

export const startApScan = { type: 'settings/startApScan' } as const
export const stopApScan = { type: 'settings/stopApScan' } as const

export interface ApFound {
  type: 'settings/apFound'
  ssid: string
  mac: string
  rssi: number
  authMode: ApiAuthMode
}

export type SettingsWsRequest =
  | typeof startApScan
  | typeof stopApScan

export type SettingsWsResponse =
  | ApFound
