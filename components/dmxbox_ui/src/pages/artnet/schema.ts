import { z } from 'zod'

const MaxUniverseValue = 65535

const UniverseSchema = z.number()
  .min(0)
  .max(MaxUniverseValue)

export const ArtnetSchema = z.object({
  nativeUniverse: UniverseSchema,
  effectControlUniverse: UniverseSchema,
})

export type ArtnetFields = z.infer<typeof ArtnetSchema>
