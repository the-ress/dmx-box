import { useParams } from "react-router-dom";
import StepTimes from "./StepTimes";
import { FormProvider, useForm } from "react-hook-form";

export default function StepPage() {
  const { stepId } = useParams()
  const form = useForm()
  return <>
    <h3>Step {stepId}</h3>
    <FormProvider {...form}>
      <StepTimes />
      <div className="touch-pan-x overflow-x-auto">
        WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
      </div>
    </FormProvider>
  </>
}
