import { Outlet, useParams } from "react-router-dom";

export default function EffectPage() {
  const { effectId } = useParams()
  return (
    <>
      <h1>Effect {effectId}</h1>
      <Outlet />
    </>
  )
}
