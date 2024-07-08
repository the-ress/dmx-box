import { createContext, useContext } from "react";
import { SettingsWsRequest, SettingsWsResponse } from "./wifi";
import useWebSocket from "react-use-websocket";

export type ApiWsRequest =
  | SettingsWsRequest

export type ApiWsResponse =
  | SettingsWsResponse
  | { type: "foo" }

export interface Api {
  readonly apiUrl: URL
  readonly wsUrl: string
}

export const ApiContext = createContext<Api>(undefined)

export function createApi(serverUrl: URL): Api {
  const apiUrl = new URL('api/', serverUrl)
  const wsUrl = new URL('ws', apiUrl)
  wsUrl.protocol = wsUrl.protocol.replace(/^http/, 'ws')
  return { apiUrl, wsUrl: wsUrl.toString() }
}

export function useDmxboxApi() {
  const { apiUrl } = useContext(ApiContext)
  return { apiUrl }
}

export function useDmxboxWebSocket() {
  const { wsUrl } = useContext(ApiContext)
  return useWebSocket<ApiWsResponse>(wsUrl, { share: true })
}
