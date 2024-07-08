import { WiFiChannel, WiFiFields, WiFiSecurityType } from "./schema"
import { ApiModel, ApiAuthMode } from "../../api/wifi"

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

export function apiModelToFields(model: ApiModel): WiFiFields {
  return {
    hostName: model.hostname,
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

export function apiModelFromFields(fields: WiFiFields): ApiModel {
  return {
    hostname: fields.hostName,
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

