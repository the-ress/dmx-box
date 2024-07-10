import { useParams } from "react-router-dom";
import StepTimes from "./StepTimes";
import { FormProvider, useForm } from "react-hook-form";

interface StepChannelProps {
  number: number
  level: number
}

function StepChannel({ number, level }: StepChannelProps) {
  return (
    <div className="min-w-16 h-full flex flex-col snap-start">
      <h3 className="flex-none">Channel {number}</h3>
      <div className="grow flex flex-col-reverse">
        <div className="m-2 bg-rose-600" style={{flexBasis: `${level}%`}}>
        </div>
      </div>
    </div>
  )
}

export default function StepPage() {
  const { stepId } = useParams()
  const form = useForm()
  return <div className="h-full flex flex-col">
    <h3 className="flex-none">Step {stepId}</h3>
    <FormProvider {...form}>
      <div className="flex-none">
        <StepTimes />
      </div>
      <div className="grow snap-x snap-mandatory touch-pan-x overflow-x-auto overflow-y-hidden flex">
        <StepChannel number={2} level={12}/>
        <StepChannel number={8} level={0} />
        <StepChannel number={9} level={24} />
        <StepChannel number={10} level={70} />
        <StepChannel number={13} level={40}/>
        <StepChannel number={14} level={15} />
        <StepChannel number={16} level={60} />
        <StepChannel number={20} level={100} />
        <StepChannel number={50} level={33} />
      </div>
    </FormProvider>
  </div>
}
