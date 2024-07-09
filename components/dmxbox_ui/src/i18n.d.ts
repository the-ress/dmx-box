import 'i18next'
import resources from './resources/en.json'

declare module 'i18next' {
  interface CustomTypeOptions {
    resources: typeof resources
  }
}
