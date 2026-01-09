import { createTask,destroyTask,getShortestPathToGCRoot } from "libleakguard.so"
import { taskpool, util } from "@kit.ArkTS"
import hilog from "@ohos.hilog"
import { AnalysisTask, NodeRef } from "./db/DatabaseInterfaces"
import { hidebug } from "@kit.PerformanceAnalysisKit"
import { systemDateTime } from "@kit.BasicServicesKit"
import { appDatabase } from "./db/AppDatabase"
import { LeakNotification } from "./LeakNotification"
import { NodeInfo } from "./model/NodeInfo"


export function analyze(objects: NodeInfo[]) {
  const context = getContext().getApplicationContext()
  const time = systemDateTime.getTime()
  const file = time+"-泄漏"
  let taskInfo:AnalysisTask = {
    isViewed:false,
    status:1,
    createTime:new Date(time),
    heapSnapshotPath:context.filesDir+'/'+file+'.heapsnapshot'
  }
  appDatabase.analysisTaskDao.insert(taskInfo).then(()=>{
    try {
      hidebug.dumpJsHeapData(file)
    } catch (error) {
      taskInfo.status = 3
      appDatabase.analysisTaskDao.update(taskInfo).catch(() => {
        hilog.error(0x0002, "Analyze", "update taskInfo error")
      })
      LeakNotification.getInstance().publishNotification(file+" 分析失败")
      return
    }
    let task = new taskpool.Task(analyzeHash,taskInfo.heapSnapshotPath,objects)
    taskpool.execute(task,taskpool.Priority.HIGH).then((nodeRefs)=>{
      hilog.debug(0x0002, "Analyze","analyzeHash done")
      taskInfo.status = 2
      taskInfo.referencePaths = nodeRefs as Array<NodeRef>
      taskInfo.completeTime = new Date()
      appDatabase.analysisTaskDao.update(taskInfo).catch(() => {
        hilog.error(0x0002, "Analyze", "update taskInfo error")
      })
      LeakNotification.getInstance().publishNotification(file+" 分析成功")
    }).catch(() => {
      hilog.error(0x0002, "Analyze", "analyzeHash error")
      taskInfo.status = 3
      appDatabase.analysisTaskDao.update(taskInfo).catch(() => {
        hilog.error(0x0002, "Analyze", "update taskInfo error")
      })
      LeakNotification.getInstance().publishNotification(file+" 分析失败")
    })
  })


}

@Concurrent
function analyzeHash(file: string, objects: NodeInfo[]):Array<NodeRef> {
  // 按name字段对objects进行分组
  const groupedObjects = new Map<string, NodeInfo[]>()
  for (let obj of objects) {
    if (!groupedObjects.has(obj.name)) {
      groupedObjects.set(obj.name, [])
    }
    groupedObjects.get(obj.name)!.push(obj)
  }
  const task = createTask(file)
  const nodeRefs:Array<NodeRef> = []
  groupedObjects.forEach((value:NodeInfo[],key:string)=>{
    const objsRef = getShortestPathToGCRoot(task, key)
    nodeRefs.push(...objsRef.map<NodeRef>((it,index:number)=>{
      const node = value[index]
      const nodeRef:NodeRef = {
      hash:node?.hash ?? 0,
      name:key,
      ref:it
    }
    return nodeRef
  }))
  })
  destroyTask(task)
  return nodeRefs
}