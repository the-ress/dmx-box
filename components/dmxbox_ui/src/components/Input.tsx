import { useFormContext } from "react-hook-form"

export interface InputProps {
  name: string
  placeholder?: string
  type?: 'text' | 'password' | 'number'
  maxLength?: number
  inputMode?: HTMLInputElement['inputMode']
}

export default function Input({ name, ...props }: InputProps) {
  const { register } = useFormContext()
  const className = `
    w-full 
    shadow-inner border border-zinc-400 rounded
    text-zinc-900
    placeholder:text-zinc-400
    px-3 py-2
  `

  return (
    <input className={className} {...props} {...register(name)} />
  )
}
