import FormRowInput from "../../components/FormRowInput"
import { useTranslation } from "react-i18next"
import { FormProvider, useForm } from "react-hook-form"
import { zodResolver } from "@hookform/resolvers/zod"
import { WifiPasswordSchema, WifiPasswordFields } from "./schema"

export interface WiFiPasswordFormProps {
  onSubmit: (fields: WifiPasswordFields) => void
}

export default function WiFiForm({ onSubmit }: WiFiPasswordFormProps) {
  const { t } = useTranslation('WiFi');

  const form = useForm<WifiPasswordFields>({
    resolver: zodResolver(WifiPasswordSchema)
  })

  return (
    <FormProvider {...form}>
      <form onSubmit={form.handleSubmit(onSubmit)}>
        <div className="flex flex-row">
          <div className="grow-0">
            <button type="submit">{t('WiFi:save')}</button>
          </div>
        </div>
        <FormRowInput
          label={t(`WiFi:security.password`)}
          name="password"
          placeholder={t(`WiFi:security.password.placeholder`)}
          type="password"
        />

      </form>
    </FormProvider>
  )
}

