import { objWatch } from './src/main/ets/ObjWatch'
import { analyze, getDumpInfo } from './src/main/ets/Analyze'
import './src/main/ets/pages/LeakPage'
import { sysWatch } from './src/main/ets/SysWatch'
import { autoWatch } from './src/main/ets/AutoWatch'

objWatch.analyzeHeapSnapshot = analyze
sysWatch.analyzeHeapSnapshot = analyze
autoWatch.getDumpInfo = getDumpInfo

export { LeakGuard,WatchLevel } from './src/main/ets/LeakGuard';
export { LEAK_TASK_ROUTE_NAME } from './src/main/ets/Constants'
