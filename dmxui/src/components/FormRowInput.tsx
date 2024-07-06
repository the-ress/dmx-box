import FormRow, { FormRowProps } from "./FormRow";
import Input, { InputProps } from "./Input";

export interface FormRowInputProps extends InputProps, Omit<FormRowProps, 'children'> {
}

export default function FormRowInput({ label, hidden, name, ...inputProps }: FormRowInputProps) {
  return (
    <FormRow hidden={hidden} label={label} name={name}>
      <Input name={name} {...inputProps} />
    </FormRow>
  )
}
