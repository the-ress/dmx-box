import FormRow, { FormRowProps } from "./FormRow.tsx"
import Select, { SelectProps } from "./Select.tsx"

export interface FormRowSelectProps extends SelectProps, Omit<FormRowProps, 'children'> {
}

export default function FormRowSelect({ comment, hidden, label, name, ...selectProps }: FormRowSelectProps) {
  return (
    <FormRow comment={comment} hidden={hidden} label={label} name={name}>
      <Select name={name} {...selectProps} />
    </FormRow>
  )
}
