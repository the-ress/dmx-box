import { ArtnetFields, ArtnetSchema } from "./schema"
import FormRowInput from "../../components/FormRowInput"
import { useTranslation } from "react-i18next"
import { FormProvider, useForm, useWatch } from "react-hook-form"
import { zodResolver } from "@hookform/resolvers/zod"

export interface ArtnetFormProps {
  fields: ArtnetFields
  onSubmit: (fields: ArtnetFields) => void
}

export default function ArtnetForm({ fields, onSubmit }: ArtnetFormProps) {
  const { t } = useTranslation('Artnet');

  const form = useForm<ArtnetFields>({
    defaultValues: fields,
    resolver: zodResolver(ArtnetSchema)
  })

  return (
    <>
      <FormProvider {...form}>
        <form onSubmit={form.handleSubmit(onSubmit)}>
          <div className="flex flex-row">
            <h1 className="grow mb-4 text-3xl select-none">{t('Artnet:h1')}</h1>
            <div className="grow-0">
              <button type="submit">{t('Artnet:save')}</button>
            </div>
          </div>

          <div className="mb-6">
            <FormRowInput
              label={t('Artnet:nativeUniverse')}
              name="nativeUniverse"
              placeholder={t('Artnet:nativeUniverse.placeholder')}
              type="text"
            />
          </div>

          <div className="mb-6">
            <FormRowInput
              label={t('Artnet:effectControlUniverse')}
              name="effectControlUniverse"
              placeholder={t('Artnet:effectControlUniverse.placeholder')}
              type="text"
            />
          </div>
        </form>
      </FormProvider>
    </>
  )
}

