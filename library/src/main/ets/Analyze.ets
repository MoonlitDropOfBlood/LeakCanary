import { createTask,destroyTask,getShortestPathToGCRoot, rawHeapTranslate, ReferenceChain } from "libleakguard.so"
import { collections, taskpool, uri } from "@kit.ArkTS"
import hilog from "@ohos.hilog"
import { NodeRef } from "./db/DatabaseInterfaces"
import { appDatabase } from "./db/AppDatabase"
import { LeakNotification } from "./LeakNotification"
import { NodeInfo, TransferData } from "./model/TransferData"
import { NodeRefShare, ReferenceChainNodeShare, ReferenceChainShare, ResultData } from "./model/ResultData"
import { CheckTask } from "./model/CheckTask"
import { fileIo } from "@kit.CoreFileKit"
import { LeakInfo } from "./model/ObjInfo"


export function analyze(checkTask:CheckTask):Promise<void> {
  const taskInfo = checkTask.task
  const fileUri = new uri.URI(taskInfo.heapSnapshotPath)
  const file = fileUri.getLastSegment().replace('.heapsnapshot','')
  const task = new taskpool.Task(analyzeHash,new TransferData(collections.Array.from(checkTask.objInfos.map(it=>new NodeInfo(it.name,it.hash))),taskInfo.heapSnapshotPath))
  return taskpool.execute(task,taskpool.Priority.HIGH).then((nodeRefs)=>{
    hilog.debug(0x0002, "Analyze","analyzeHash done")
    taskInfo.status = 2
    taskInfo.referencePaths = shareToObject(nodeRefs as ResultData)
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
}

export async function getDumpInfo(checkTask:CheckTask){
  const taskInfo = checkTask.task
  const fileUri = new uri.URI(taskInfo.heapSnapshotPath)
  const file = fileUri.getLastSegment().replace('.rawheap','')
  const task = new taskpool.Task(rawAnalyzeHash,new TransferData(collections.Array.from(checkTask.objInfos.map(it=>new NodeInfo(it.name,it.hash))),taskInfo.heapSnapshotPath))
  return taskpool.execute(task,taskpool.Priority.HIGH).then((nodeRefs)=>{
    hilog.debug(0x0002, "Analyze","analyzeHash done")
    taskInfo.status = 2
    taskInfo.referencePaths = shareToObject(nodeRefs as ResultData)
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
}

@Concurrent
async function rawAnalyzeHash(transfer: TransferData):Promise<ResultData> {
  const file = transfer.file.replace('rawheap','heapsnapshot')
  rawHeapTranslate(transfer.file,transfer.file.replace('rawheap','heapsnapshot'))
  transfer.file = file
  let task:number = -1
  try {
    task = createTask(transfer.file)
    const nodeRefs: Array<NodeRefShare> = []
    transfer.nodeInfos.forEach((it)=>{
      const ref = getShortestPathToGCRoot(task, `Int:${it.hash}`)
      if(ref.length > 0) {
        const refs = new collections.Array<ReferenceChainShare>()
        ref.forEach(item => refs.push(new ReferenceChainShare(
          new ReferenceChainNodeShare(item.from.nodeId, item.from.name, item.from.type, item.from.path, item.from.line),
          item.edgeType,
          new ReferenceChainNodeShare(item.to.nodeId, item.to.name, item.to.type, item.to.path, item.to.line)
        )))
        nodeRefs.push(new NodeRefShare(it.hash, it.name, refs))
      }
    })
    return new ResultData(collections.Array.from(nodeRefs))
  } finally {
    if(task != -1){
      destroyTask(task)
    }
    // fileIo.unlinkSync(transfer.file)
  }
}

/**
 * 跨线程对象转成普通对象
 * @param result
 * @returns
 */
function shareToObject(result: ResultData):NodeRef[] {
  const nodes:NodeRef[] = []
  result.nodeRefs.forEach((ite:NodeRefShare)=>{
    const refs:ReferenceChain[] = []
    ite.ref.forEach((item=>{
      refs.push({
        from: {
          nodeId:item.from.nodeId,
          name:item.from.name,
          type:item.from.type,
          path:item.from.path,
          line:item.from.line,
        },
        edgeType:item.edgeType,
        to:{
          nodeId:item.to.nodeId,
          name:item.to.name,
          type:item.to.type,
          path:item.to.path,
          line:item.to.line,
        }
      })
    }))
    nodes.push({
      hash:ite.hash,
      name:ite.name,
      ref:refs
    })
  })
  return nodes
}

@Concurrent
function analyzeHash(transfer: TransferData):ResultData {
  let task:number = -1
  try {
    task = createTask(transfer.file)
    const nodeRefs: Array<NodeRefShare> = []
    transfer.nodeInfos.forEach((it)=>{
      const ref = getShortestPathToGCRoot(task, `Int:${it.hash}`)
      if(ref.length > 0) {
        const refs = new collections.Array<ReferenceChainShare>()
        ref.forEach(item => refs.push(new ReferenceChainShare(
          new ReferenceChainNodeShare(item.from.nodeId, item.from.name, item.from.type, item.from.path, item.from.line),
          item.edgeType,
          new ReferenceChainNodeShare(item.to.nodeId, item.to.name, item.to.type, item.to.path, item.to.line)
        )))
        nodeRefs.push(new NodeRefShare(it.hash, it.name, refs))
      }
    })
    return new ResultData(collections.Array.from(nodeRefs))
  } finally {
    if(task != -1){
      destroyTask(task)
    }
  }
}