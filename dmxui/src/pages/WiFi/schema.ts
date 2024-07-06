import { z } from 'zod'

export const WiFiSecurityTypeSchema = z.enum(["none", "wep", "wpa", "wpa23", "wpa3"])
export const WiFiSecurityType = WiFiSecurityTypeSchema.enum
export type WiFiSecurityType = z.infer<typeof WiFiSecurityTypeSchema>

const MinNetworkNameLength = 1
const MinPasswordLength = 8
export const MaxPasswordLength = 63
export const MaxNetworkNameLength = 32

export const WiFiSecuritySchema = z.discriminatedUnion('type', [
  z.object({
    type: z.literal(WiFiSecurityType.none),
    password: z.string().optional(),
  }),
  z.object({
    type: WiFiSecurityTypeSchema.exclude([WiFiSecurityType.none]),
    password: z.string().min(MinPasswordLength).max(MaxPasswordLength),
  }),
])

const WiFiSecurityInactiveSchema = z.object({
  type: WiFiSecurityTypeSchema,
  password: z.string().optional()
})

const WiFiChannelSchema = z.enum([
  'auto', '1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12', '13',
])

const NetworkNameSchema = z.string()
  .min(MinNetworkNameLength)
  .max(MaxNetworkNameLength)

export const WiFiChannel = WiFiChannelSchema.enum

export type WiFiChannel = typeof WiFiChannel.auto

const AccessPointSchema = z.object({
  name: NetworkNameSchema,
  security: WiFiSecuritySchema,
  channel: WiFiChannelSchema.default(WiFiChannel.auto),
})

const ExistingNetworkSchema = z.discriminatedUnion('enabled', [
  z.object({
    enabled: z.literal(true),
    name: NetworkNameSchema,
    security: WiFiSecuritySchema,
  }),
  z.object({
    enabled: z.literal(false),
    name: z.string().optional(),
    security: WiFiSecurityInactiveSchema,
  })
])

export const WiFiSchema = z.object({
  accessPoint: AccessPointSchema,
  existingNetwork: ExistingNetworkSchema,
})

export type WiFiFields = z.infer<typeof WiFiSchema>
