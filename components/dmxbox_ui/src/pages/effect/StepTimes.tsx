import Input from "../../components/Input"

interface DurationInputProps {
  name: string
  label: string
}

function DurationInput({ label, name }: DurationInputProps) {
  return <Input
    name={name}
    placeholder={label}
  />
}

export default function StepTimes() {
  return <>
    <div className="flex flex-row">
      <DurationInput name="time" label="time" />
      <DurationInput name="in" label="in" />
      <DurationInput name="dwell" label="dwell" />
      <DurationInput name="out" label="out" />
    </div>
    <div className="touch-pan-x overflow-x">
      WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
    </div>
  </>
}
