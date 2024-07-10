import Input from "../../components/Input"

interface DurationInputProps {
  name: string
  label: string
}

function DurationInput({ label, name }: DurationInputProps) {
  return (
    <div className="mx-1">
      <Input
        name={name}
        placeholder={label}
      />
    </div>
  )
}

export default function StepTimes() {
  return <>
    <div className="flex flex-row">
      <DurationInput name="time" label="time" />
      <DurationInput name="in" label="in" />
      <DurationInput name="dwell" label="dwell" />
      <DurationInput name="out" label="out" />
    </div>
  </>
}
