import { useParams } from "react-router-dom";
import StepTimes from "./StepTimes";
import { FormProvider, useFieldArray, useForm } from "react-hook-form";
import Input from "../../components/Input";

interface StepChannelProps {
  name: string
}

function StepChannel({ name }: StepChannelProps) {
  return (
  <div className="snap-start flex flex-row gap-x-1 mb-1">
    <h3 className="grow flex">
      <Input
        name={`${name}.number`}
        type="number"
        min="1"
        max="512"
        inputMode="numeric" />
    </h3>
    <div className="flex-none">
      <Input
        name={`${name}.level`}
        inputMode="numeric"
      />
    </div>
  </div>
)
}

function StepChannelList({ name }: { name: string }) {
  const { fields } = useFieldArray({ name })
  return fields.map((field, index) => (
    <StepChannel key={field.id} name={`${name}.${index}`} />
  ))
}

export default function Step"age() {
const { effectId, stepId } = useParams()
const form = useForm({
  defaultValues: {
    channels: [
      { number: 5, level: 20 },
      { number: 6, level: 83 },
      { number: 12, level: 11 },
      { number: 20, level: 64 },
      { number: 500, level: 100 },
      { number: 511, level: 255 },
    ]
  }
})

return <div className="h-full flex flex-col">
  <h3 className="flex-none">Effect {effectId} Step {stepId}</h3>
  <FormProvider {...form}>
    <div className="grow snap-y touch-pan-y overflow-y-auto p-1">
      <StepChannelList name="channels" />
    </div>
    <div className="flex-none m-1">
      <StepTimes />
    </div>
  </FormProvider>
</div>
}
