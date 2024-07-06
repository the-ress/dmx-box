import { ReactNode } from 'react'
import { ErrorMessage } from '@hookform/error-message'
import { BsExclamationCircle } from 'react-icons/bs'

export interface FormRowProps {
  comment?: string
  children: ReactNode
  hidden?: boolean
  label: string
  name: string
}

export default function FormRow({ children, comment, hidden, label, name }: FormRowProps) {
  return (
    <div className="mb-2" hidden={hidden}>
      <label className="flex flex-row text-zinc-800">
        <div className="basis-1/4 py-2 select-none">{label}</div>
        <div className="basis-3/4">
          {children}
        </div>
      </label>
      <div className="basis-3/4 flex-none select-none text-xs text-right text-zinc-600 my-1">
        <ErrorMessage
          name={name}
          render={({ message }) =>
            <span className='text-rose-700'>
              <BsExclamationCircle className="inline-block align-middle" />
              &nbsp;{message}
            </span>
          }
        />
        {comment && <div>{comment}</div>}
      </div>
    </div>
  );
}

