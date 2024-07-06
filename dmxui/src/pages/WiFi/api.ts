import { Md11Mp } from 'react-icons/md'
import { WiFiSecurityType, WiFiFields, WiFiChannel } from './schema'

type ApiAuthMode =
  | 'open'
  | 'WEP'
  | 'WPA_PSK'
  | 'WPA_WPA2_PSK'
  | 'WPA2_PSK'
  | 'WPA2_WPA3_PSK'
  | 'WPA3_PSK'

interface ApiModel {
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

function apiAuthModeToForm(authMode: ApiAuthMode): WiFiSecurityType {
  switch (authMode) {
    case 'open':
      return WiFiSecurityType.none
    case 'WEP':
      return WiFiSecurityType.wep
    case 'WPA_PSK':
    case 'WPA_WPA2_PSK':
      return WiFiSecurityType.wpa
    case 'WPA3_PSK':
      return WiFiSecurityType.wpa3
  }
  return WiFiSecurityType.wpa23
}

function apiAuthModeFromForm(type: WiFiSecurityType): ApiAuthMode {
  switch (type) {
    case WiFiSecurityType.none:
      return 'open'
    case WiFiSecurityType.wep:
      return 'WEP'
    case WiFiSecurityType.wpa:
      return 'WPA_WPA2_PSK'
    case WiFiSecurityType.wpa3:
      return 'WPA3_PSK'
  }
  return 'WPA2_WPA3_PSK'
}

function apiModelToFields(model: ApiModel): WiFiFields {
  return {
    accessPoint: {
      name: model.ap.ssid,
      security: {
        type: apiAuthModeToForm(model.ap.auth_mode),
        password: model.ap.password
      },
      channel: model.ap.channel.toString() as WiFiChannel
    },
    existingNetwork: {
      enabled: model.sta.enabled,
      name: model.sta.ssid,
      security: {
        type: apiAuthModeToForm(model.sta.auth_mode),
        password: model.sta.password
      },
    },
  }
}

function apiModelFromFields(fields: WiFiFields): ApiModel {
  return {
    ap: {
      ssid: fields.accessPoint.name,
      auth_mode: apiAuthModeFromForm(fields.accessPoint.security.type),
      password: fields.accessPoint.security.password,
      channel: fields.accessPoint.channel === WiFiChannel.auto
        ? 11
        : parseInt(fields.accessPoint.channel, 10)
    },
    sta: {
      enabled: fields.existingNetwork.enabled,
      ssid: fields.existingNetwork.name,
      auth_mode: apiAuthModeFromForm(fields.existingNetwork.security.type),
      password: fields.existingNetwork.security.password,
    }
  }

}

const server = window.location.hash ? window.location.hash.substring(1) : ''
const wifiEndpoint = `${server}/api/wifi-config`

export async function loadWiFiConfig(): Promise<WiFiFields> {
  const response = await fetch(wifiEndpoint)
  if (!response.ok) {
    throw await response.text()
  }
  const model = await response.json()
  return apiModelToFields(model)
}

export async function submitWiFiConfig(fields: WiFiFields): Promise<void> {
  const model = apiModelFromFields(fields)
  const response = await fetch(wifiEndpoint, {
    method: 'PUT',
    body: JSON.stringify(model)
  })
  if (!response.ok) {
    throw await response.text()
  }
}
