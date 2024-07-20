import { ArtnetFields } from "./schema"
import { ApiModel } from "../../api/artnet"

export function apiModelToFields(model: ApiModel): ArtnetFields {
  return {
    nativeUniverse: model.native_universe,
    effectControlUniverse: model.effect_control_universe,
  }
}

export function apiModelFromFields(fields: ArtnetFields): ApiModel {
  return {
    native_universe: fields.nativeUniverse,
    effect_control_universe: fields.effectControlUniverse,
  }
}
