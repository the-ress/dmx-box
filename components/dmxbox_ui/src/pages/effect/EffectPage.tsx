import { Outlet, useParams } from "react-router-dom";

export default function EffectPage() {
  return (
    <div className="h-dvh flex flex-col">
      <Outlet />
    </div>
  )
}
