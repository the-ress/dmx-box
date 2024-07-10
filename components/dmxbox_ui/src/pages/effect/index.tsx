import { RouteObject } from 'react-router-dom'
import LinkWithSearchParams from '../../components/LinkWithSearchParams'
import EffectPage from "./EffectPage"
import EffectEditor from './EffectEditor'
import StepPage from './StepPage'

export function effectRoutes(path: string): RouteObject[] {
  return [
    { path, element: <LinkWithSearchParams to="effects/1">Effect 1</LinkWithSearchParams> },
    {
      path: `${path}/:effectId`,
      Component: EffectPage,
      children: [
        { path: '', Component: EffectEditor },
        { path: `steps/:stepId`, Component: StepPage },
      ],
    },
  ]
}

