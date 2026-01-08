import { objWatch } from './src/main/ets/ObjWatch'
import { analyze } from './src/main/ets/Analyze'
import './src/main/ets/pages/LeakPage'

objWatch.analyzeHeapSnapshot = analyze

export { LeakCanary } from './src/main/ets/LeakCanary';
export { objWatch }
