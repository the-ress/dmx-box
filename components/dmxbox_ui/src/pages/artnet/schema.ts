import { z } from 'zod'

// TODO

// const MaxUniverseValue = 65535
// const UniverseSchema = z.number()
//   .min(0)
//   .max(MaxUniverseValue)

const UniverseSchema = z.string()

export const ArtnetSchema = z.object({
  nativeUniverse: UniverseSchema,
  effectControlUniverse: UniverseSchema,
})

export type ArtnetFields = z.infer<typeof ArtnetSchema>
