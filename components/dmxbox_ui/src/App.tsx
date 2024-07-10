import { Outlet } from "react-router-dom";
import { ApiProvider } from "./api";

export default function App() {
  return (
    <ApiProvider>
      <Outlet />
    </ApiProvider>
  )
}
