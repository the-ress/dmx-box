import { useFormContext } from "react-hook-form";

export interface FormHeaderProps {
  checkboxName?: string
  heading: string
  subHeading?: string
}

export default function FormHeader({ checkboxName, heading, subHeading }: FormHeaderProps) {
  const { register } = useFormContext()
  return (
    <div className="mb-4">
      <label className="flex flex-row text-zinc-800">
        {checkboxName && 
          <input
            className="rounded w-5 h-5 my-3 mr-3"
            type="checkbox"
            {...register(checkboxName)}
          />
        }
        <div className="select-none">
          <h2 className="text-lg">{heading}</h2>
          <p className="text-sm text-zinc-500">
            {subHeading}
          </p>
        </div>
      </label>
    </div>
  );
};
