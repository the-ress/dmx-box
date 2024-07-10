import { Outlet, useParams } from "react-router-dom";

export default function EffectPage() {
  const { effectId } = useParams()
  return (
    <div className="h-dvh flex flex-col">
      <h1 className="flex-none">Effect {effectId}</h1>
      <Outlet />
    </div>
  )
}
