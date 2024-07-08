import { createContext } from "react";
import useWebSocket from "react-use-websocket"
import { ApiModel, loadWiFiConfig, SettingsWsRequest, SettingsWsResponse, submitWiFiConfig } from "./wifi";

export type ApiWsRequest =
  | SettingsWsRequest

export type ApiWsResponse =
  | SettingsWsResponse
  | { type: "foo" }

export interface Api {
  lastWsResponse?: ApiWsResponse
  sendWsRequest(wsRequest: ApiWsRequest): void

  fetchSettings(): Promise<ApiModel>
  submitSettings(settings: ApiModel): Promise<void>
}

export const ApiContext = createContext<Api>(undefined)

export function createApi(serverUrl: URL): Api {
  const apiUrl = new URL('api/', serverUrl)
  const wsUrl = new URL('ws', apiUrl)
  wsUrl.protocol = wsUrl.protocol.replace(/^http/, 'ws')

  const ws = useWebSocket<ApiWsResponse>(wsUrl.toString())
  return {
    get lastWsResponse() { return ws.lastJsonMessage as ApiWsResponse },
    sendWsRequest: ws.sendJsonMessage<ApiWsRequest>,
    fetchSettings: loadWiFiConfig.bind(null, apiUrl),
    submitSettings: submitWiFiConfig.bind(null, apiUrl),
  }
}
