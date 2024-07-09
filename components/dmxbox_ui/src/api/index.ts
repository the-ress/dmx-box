import { createContext, useContext, useEffect } from "react";
import { SettingsWsRequest, SettingsWsResponse } from "./wifi";
import useWebSocket from "react-use-websocket";

export type WsRequest =
  | SettingsWsRequest

export type WsResponse =
  | SettingsWsResponse

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

export function useApi() {
  const { apiUrl } = useContext(ApiContext)
  return { apiUrl }
}

export function useWs() {
  const { wsUrl } = useContext(ApiContext)
  return useWebSocket<WsResponse>(wsUrl, { share: true })
}

export function useLastWsResponse<Type extends WsResponse['type']>(
  type: Type,
  effect: (response: Extract<WsResponse, { type: Type }>) => void
): void {
  const { lastJsonMessage } = useWs()
  useEffect(() => {
    if (lastJsonMessage?.type === type) {
      effect(lastJsonMessage as Extract<WsResponse, { type: Type }>)
    }
  }, [lastJsonMessage])
}

