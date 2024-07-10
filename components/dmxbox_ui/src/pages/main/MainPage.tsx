import LinkWithSearchParams from "../../components/LinkWithSearchParams";

export default function MainPage() {
  return (
    <ul>
      <li><LinkWithSearchParams to="/settings">Settings</LinkWithSearchParams></li>
      <li><LinkWithSearchParams to="/effects/1">Effect</LinkWithSearchParams></li>
    </ul>
  )
}
