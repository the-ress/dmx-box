import { MaxHostNameLength, MaxNetworkNameLength, WiFiChannel, WiFiFields, WiFiSchema } from "./schema"
import WiFiSecurity from "./WiFiSecurity"
import FormHeader from "../../components/FormHeader"
import FormRowInput from "../../components/FormRowInput"
import FormRowSelect from "../../components/FormRowSelect"
import { useTranslation } from "react-i18next"
import { FormProvider, useForm, useWatch } from "react-hook-form"
import { zodResolver } from "@hookform/resolvers/zod"

export interface WiFiFormProps {
  fields: WiFiFields
  onSubmit: (fields: WiFiFields) => void
}

export default function WiFiForm({ fields, onSubmit }: WiFiFormProps) {
  const { t } = useTranslation('WiFi');
  const FieldNameJoinExistingEnabled = 'existingNetwork.enabled'

  const WiFiChannelOptions = [
    { value: WiFiChannel.auto, label: t('WiFi:accessPoint.channel.auto') },
    { value: WiFiChannel[1], label: t('WiFi:accessPoint.channel.1') },
    { value: WiFiChannel[2], label: t('WiFi:accessPoint.channel.2') },
    { value: WiFiChannel[3], label: t('WiFi:accessPoint.channel.3') },
    { value: WiFiChannel[4], label: t('WiFi:accessPoint.channel.4') },
    { value: WiFiChannel[5], label: t('WiFi:accessPoint.channel.5') },
    { value: WiFiChannel[6], label: t('WiFi:accessPoint.channel.6') },
    { value: WiFiChannel[7], label: t('WiFi:accessPoint.channel.7') },
    { value: WiFiChannel[8], label: t('WiFi:accessPoint.channel.8') },
    { value: WiFiChannel[9], label: t('WiFi:accessPoint.channel.9') },
    { value: WiFiChannel[10], label: t('WiFi:accessPoint.channel.10') },
    { value: WiFiChannel[11], label: t('WiFi:accessPoint.channel.11') },
    { value: WiFiChannel[12], label: t('WiFi:accessPoint.channel.12') },
    { value: WiFiChannel[13], label: t('WiFi:accessPoint.channel.13') },
  ]

  const form = useForm<WiFiFields>({
    defaultValues: fields,
    resolver: zodResolver(WiFiSchema)
  })

  const joinExisting = useWatch({
    control: form.control,
    name: FieldNameJoinExistingEnabled
  })

  return (
    <FormProvider {...form}>
      <form onSubmit={form.handleSubmit(onSubmit)}>
        <div className="flex flex-row">
          <h1 className="grow mb-4 text-3xl select-none">{t('WiFi:h1')}</h1>
          <div className="grow-0">
            <button type="submit">{t('WiFi:save')}</button>
          </div>
        </div>

        <div className="mb-6">
          <FormRowInput
            label={t('WiFi:hostName')}
            maxLength={MaxHostNameLength}
            name="hostName"
            placeholder={t('WiFi:hostName.placeholder')}
            type="text"
          />
        </div>

        <FormHeader
          heading={t('WiFi:accessPoint.heading')}
          subHeading={t('WiFi:accessPoint.subheading')}
        />

        <FormRowInput
          label={t('WiFi:accessPoint.name')}
          maxLength={MaxNetworkNameLength}
          name="accessPoint.name"
          placeholder={t('WiFi:accessPoint.name.placeholder')}
          type="text"
        />

        <WiFiSecurity
          name="accessPoint.security"
          noneComment={t('WiFi:accessPoint.security.noneComment')}
          insecureComment={t('WiFi:accessPoint.security.insecureComment')}
        />

        <FormRowSelect
          comment={joinExisting && t('WiFi:accessPoint.channel.joinedComment')}
          defaultValue={WiFiChannel.auto}
          disabled={joinExisting}
          label={t('WiFi:accessPoint.channel')}
          name="accessPoint.channel"
          options={WiFiChannelOptions}
        />

        <div className="mt-6">
          <FormHeader
            checkboxName={FieldNameJoinExistingEnabled}
            heading={t('WiFi:existingNetwork.heading')}
            subHeading={t('WiFi:existingNetwork.subHeading')}
          />
        </div>

        <div hidden={!joinExisting}>
          <FormRowInput
            label={t('WiFi:existingNetwork.name')}
            name="existingNetwork.name"
            placeholder={t('WiFi:existingNetwork.name.placeholder')}
            type="text"
          />

          <WiFiSecurity
            name="existingNetwork.security"
            noneComment={t('WiFi:existingNetwork.security.noneComment')}
            insecureComment={t('WiFi:existingNetwork.security.insecureComment')}
          />
        </div>

      </form>
    </FormProvider>
  )
}

