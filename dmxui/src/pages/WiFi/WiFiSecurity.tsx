import { useWatch } from 'react-hook-form'
import FormRowSelect from '../../components/FormRowSelect.tsx'
import FormRowInput from '../../components/FormRowInput.tsx'
import { SelectOptionObject } from '../../components/Select.tsx'
import { WiFiSecurityType } from './schema.ts'
import { useTranslation } from 'react-i18next'

export interface WiFiSecurityProps {
  name: string
  noneComment?: string
  insecureComment?: string
}

const DefaultSecurityType = WiFiSecurityType.wpa23

export default function WiFiSecurity(
  { name, noneComment, insecureComment }: WiFiSecurityProps
) {
  const { t } = useTranslation()

  const SecurityTypeOptions: SelectOptionObject[] = [
    { label: t('WiFi:security.type.none'), value: WiFiSecurityType.none },
    { label: t('WiFi:security.type.wep'), value: WiFiSecurityType.wep },
    { label: t('WiFi:security.type.wpa'), value: WiFiSecurityType.wpa },
    { label: t('WiFi:security.type.wpa23'), value: WiFiSecurityType.wpa23 },
    { label: t('WiFi:security.type.wpa3'), value: WiFiSecurityType.wpa3 },
  ]

  const typeField = `${name}.type`
  const passwordField = `${name}.password`

  const type = useWatch({
    name: typeField,
    defaultValue: DefaultSecurityType
  })

  const typeComment =
    type === WiFiSecurityType.none ? noneComment :
    type === WiFiSecurityType.wep ? insecureComment :
    type === WiFiSecurityType.wpa ? insecureComment :
    undefined

  return (
    <div>
      <FormRowSelect
        comment={typeComment}
        defaultValue={DefaultSecurityType}
        label={t(`WiFi:security.type`)}
        name={typeField}
        options={SecurityTypeOptions}
      />
      <FormRowInput
        hidden={type === WiFiSecurityType.none}
        label={t(`WiFi:security.password`)}
        name={passwordField}
        placeholder={t(`WiFi:security.password.placeholder`)}
        type="password"
      />
    </div>
  )
}
