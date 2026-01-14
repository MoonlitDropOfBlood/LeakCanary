import { createTask,destroyTask,getShortestPathToGCRoot, rawHeapTranslate } from "libleakguard.so"
import { HashSet, taskpool, util } from "@kit.ArkTS"
import hilog from "@ohos.hilog"
import { AnalysisTask, NodeRef } from "./db/DatabaseInterfaces"
import { hidebug } from "@kit.PerformanceAnalysisKit"
import { systemDateTime } from "@kit.BasicServicesKit"
import { appDatabase } from "./db/AppDatabase"
import { LeakNotification } from "./LeakNotification"
import { NodeInfo } from "./model/NodeInfo"
import { NodeTransfer } from "./model/NodeTransfer"


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
  const analyzeTarget = Array.from(objects.values()).map(it => new NodeInfo(it))
  return appDatabase.analysisTaskDao.insert(taskInfo).then(()=>{
      return hidebug.dumpJsRawHeapData(false)
    }).then((path:string)=>{
    let task = new taskpool.Task(analyzeHash,path,taskInfo.heapSnapshotPath,analyzeTarget.map(it=>{
      const nodeTransfer:NodeTransfer = {hash:it.hash,name:it.name}
      return nodeTransfer
    }))
    return taskpool.execute(task,taskpool.Priority.HIGH)
  }).then((nodeRefs)=>{
    hilog.debug(0x0002, "Analyze","analyzeHash done")
    taskInfo.status = 2
    taskInfo.referencePaths = nodeRefs as Array<NodeRef>
    taskInfo.completeTime = new Date()
    appDatabase.analysisTaskDao.update(taskInfo).catch(() => {
      hilog.error(0x0002, "Analyze", "update taskInfo error")
    })
    LeakNotification.getInstance().publishNotification(file+" 分析成功")
  })
    .catch((error:Error) => {
      hilog.error(0x0002, "Analyze", "analyzeHash error"+error)
    taskInfo.status = 3
    appDatabase.analysisTaskDao.update(taskInfo).catch(() => {
      hilog.error(0x0002, "Analyze", "update taskInfo error")
    })
    LeakNotification.getInstance().publishNotification(file+" 分析失败")
  })


}

@Concurrent
function analyzeHash(rawFile:string, file: string, objects: NodeTransfer[]):Array<NodeRef> {
  let task:number = -1
  try {
    rawHeapTranslate(rawFile,file)
    task = createTask(file)
    const nodeRefs: Array<NodeRef> = []
    objects.forEach((it)=>{
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