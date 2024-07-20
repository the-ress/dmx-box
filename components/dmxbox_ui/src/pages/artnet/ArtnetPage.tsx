import ArtnetForm from './ArtnetForm'
import { useEffect, useState } from 'react'
import { getArtnetConfig, putArtnetConfig } from '../../api/artnet'
import { apiModelFromFields, apiModelToFields } from './api'
import { ArtnetFields } from './schema'
import { useApi } from '../../api'

export default function ArtnetPage() {
  const { apiUrl } = useApi()

  const [fields, setFields] = useState(undefined)

  const submit = async (fields: ArtnetFields) => {
    const model = apiModelFromFields(fields);
    await putArtnetConfig(apiUrl, model);
  }

  useEffect(() => {
    getArtnetConfig(apiUrl).then(apiModelToFields).then(setFields)
  }, [apiUrl])

  return fields
    ? <ArtnetForm fields={fields} onSubmit={submit} />
    : <div>Loading</div>
}

