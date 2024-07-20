function put<T>(endpoint: string) {
  return async (baseUrl: URL, model: T) => {
    const response = await fetch(new URL(endpoint, baseUrl), {
      method: 'PUT',
      body: JSON.stringify(model)
    })
    if (!response.ok) {
      throw await response.text()
    }
  }
}

function get<T>(endpoint: string) {
  return async (baseUrl: URL): Promise<T> => {
    const response = await fetch(new URL(endpoint, baseUrl))
    if (!response.ok) {
      throw await response.text()
    }
    const model = await response.json()
    return model
  }
}
export interface ApiModel {
  native_universe: number
  effect_control_universe: number
}

export const getArtnetConfig = get<ApiModel>('settings/artnet')
export const putArtnetConfig = put<ApiModel>('settings/artnet')
