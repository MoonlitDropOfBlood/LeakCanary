import { objWatch } from './src/main/ets/ObjWatch'
import { analyze } from './src/main/ets/Analyze'
import './src/main/ets/pages/LeakPage'

objWatch.analyzeHeapSnapshot = analyze

export { LeakGuard } from './src/main/ets/LeakGuard';
export { objWatch }
export { LEAK_TASK_ROUTE_NAME } from './src/main/ets/Constants'
