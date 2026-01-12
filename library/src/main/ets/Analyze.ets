import { createTask,destroyTask,getShortestPathToGCRoot } from "libleakguard.so"
import { taskpool } from "@kit.ArkTS"
import hilog from "@ohos.hilog"
import { AnalysisTask, NodeRef } from "./db/DatabaseInterfaces"
import { hidebug } from "@kit.PerformanceAnalysisKit"
import { systemDateTime } from "@kit.BasicServicesKit"
import { appDatabase } from "./db/AppDatabase"
import { LeakNotification } from "./LeakNotification"
import { NodeInfo } from "./model/NodeInfo"


export function analyze(objects: NodeInfo[]):Promise<void> {
  const context = getContext().getApplicationContext()
  const time = systemDateTime.getTime()
  const file = time+"-泄漏"
  let taskInfo:AnalysisTask = {
    isViewed:false,
    status:1,
    createTime:new Date(time),
    heapSnapshotPath:context.filesDir+'/'+file+'.heapsnapshot'
  }
  return appDatabase.analysisTaskDao.insert(taskInfo).then(()=>{
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
    return taskpool.execute(task,taskpool.Priority.HIGH).then((nodeRefs)=>{
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
  let task:number = -1
  try {
    task = createTask(file)
    const nodeRefs: Array<NodeRef> = []
    objects.forEach((it)=>{
      const ref = getShortestPathToGCRoot(task, `Int:${it.hash}`)
        nodeRefs.push({
          name:it.name,
          hash:it.hash,
          ref
        })
    })
    return nodeRefs
  } finally {
    if(task != -1){
      destroyTask(task)
    }
  }

}