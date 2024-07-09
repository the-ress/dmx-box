import { useFormContext } from 'react-hook-form'

export interface SelectOptionObject {
  value: string
  label?: string
}

export type SelectOption = string | SelectOptionObject

export interface SelectProps {
  defaultValue?: string
  disabled?: boolean
  name: string
  options: SelectOption[]
}

function normalizeOption(option: SelectOption) {
  if (typeof option === 'string') {
    return { value: option }
  }
  return option
}

export default function Select({ defaultValue, disabled, name, options }: SelectProps) {
  const { register } = useFormContext()
  const normalizedOptions = options.map(normalizeOption)
  return (
    <select
      className="
        w-full h-full
        shadow border border-zinc-400 rounded
        bg-transparent text-zinc-900
        disabled:text-zinc-400 disabled:border-zinc-200 disabled:shadow-none
        px-3 py-2"
      defaultValue={defaultValue}
      disabled={disabled}
      {...register(name)}
    >
      {...normalizedOptions.map(({ value, label }) => (
        <option key={value} value={value}>{label ?? value}</option>
      ))}
    </select>
  )
}
