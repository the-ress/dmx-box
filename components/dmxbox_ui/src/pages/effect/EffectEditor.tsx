import LinkWithSearchParams from "../../components/LinkWithSearchParams"

export interface EffectStepProps {
  index: number
}

export interface EffectEditorProps {
  name: string
  effectId: string
}

export default function EffectEditor({effectId, name }: EffectEditorProps) {
  return (
    <div>
      <h1 className="flex-none">Effect {effectId}</h1>
      <ol>
      <li><LinkWithSearchParams to="./steps/1">Step 1</LinkWithSearchParams></li>
      </ol>
    </div>
  )
}
