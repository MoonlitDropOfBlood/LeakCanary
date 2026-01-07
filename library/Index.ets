import { objWatch } from './src/main/ets/ObjWatch'
import { analyze } from './src/main/ets/Analyze'

objWatch.analyzeHeapSnapshot = analyze

export { LeakCanary } from './src/main/ets/LeakCanary';
