import { ReactNode, useEffect } from "react"
import { useWs, WsRequest } from "../api"

export interface WsSubscriptionProps {
  start: WsRequest
  stop: WsRequest
  children: ReactNode
}

export default function WsSubscription({ children, start, stop }: WsSubscriptionProps) {
  const { sendJsonMessage } = useWs()
  useEffect(() => {
    sendJsonMessage(start)
    return () => sendJsonMessage(stop)
  }, [start, stop])
  return children
}


