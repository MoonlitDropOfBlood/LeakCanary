import { objWatch } from './src/main/ets/ObjWatch'
import { analyze } from './src/main/ets/Analyze'
import './src/main/ets/pages/LeakPage'
import { sysWatch } from './src/main/ets/SysWatch'

objWatch.analyzeHeapSnapshot = analyze
sysWatch.analyzeHeapSnapshot = analyze

export { LeakGuard } from './src/main/ets/LeakGuard';
export { LEAK_TASK_ROUTE_NAME } from './src/main/ets/Constants'
