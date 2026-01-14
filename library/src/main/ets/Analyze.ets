import { createTask,destroyTask,getShortestPathToGCRoot } from "libleakguard.so"
import { collections, HashSet, taskpool, util } from "@kit.ArkTS"
import hilog from "@ohos.hilog"
import { AnalysisTask, NodeRef } from "./db/DatabaseInterfaces"
import { hidebug } from "@kit.PerformanceAnalysisKit"
import { systemDateTime } from "@kit.BasicServicesKit"
import { appDatabase } from "./db/AppDatabase"
import { LeakNotification } from "./LeakNotification"
import { NodeInfo, TransferData } from "./model/TransferData"


export function analyze(objects: HashSet<object>):Promise<void> {
  const context = getContext().getApplicationContext()
  const time = systemDateTime.getTime()
  const file = time+"-泄漏"
  let taskInfo:AnalysisTask = {
    isViewed:false,
    status:1,
    createTime:new Date(time),
    heapSnapshotPath:context.filesDir+'/'+file+'.heapsnapshot'
  }
  const array = new collections.Array<NodeInfo>()
  objects.forEach((it:object)=>{
    array.push(new NodeInfo(it.constructor.name,util.getHash(it)))
  })
  objects.clear() // 防止 GC 出现意外的引用
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
    let task = new taskpool.Task(analyzeHash,new TransferData(array,taskInfo.heapSnapshotPath))
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
function analyzeHash(transfer: TransferData):Array<NodeRef> {
  let task:number = -1
  try {
    task = createTask(transfer.file)
    const nodeRefs: Array<NodeRef> = []
    transfer.nodeInfos.forEach((it)=>{
      const ref = getShortestPathToGCRoot(task, `Int:${it.hash}`)
      if(ref.length > 0) {
        nodeRefs.push({
          name: it.name,
          hash: it.hash,
          ref
        })
      }
    })
    return nodeRefs
  } finally {
    if(task != -1){
      destroyTask(task)
    }
  }

}