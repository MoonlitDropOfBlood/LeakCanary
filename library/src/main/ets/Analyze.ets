import {
  heapAnalyzeHash, rawAnalyzeHash } from "libleakguard.so"
import { uri } from "@kit.ArkTS"
import hilog from "@ohos.hilog"
import { appDatabase } from "./db/AppDatabase"
import { LeakNotification } from "./LeakNotification"
import { CheckTask } from "./model/CheckTask"
import { fileIo } from "@kit.CoreFileKit"

export function analyze(checkTask:CheckTask):Promise<void> {
  const taskInfo = checkTask.task
  const fileUri = new uri.URI(taskInfo.heapSnapshotPath)
  const file = fileUri.getLastSegment().replace('.heapsnapshot','')
  return heapAnalyzeHash(taskInfo.heapSnapshotPath,checkTask.objInfos).then((nodeRefs)=>{
    hilog.debug(0x0002, "Analyze","analyzeHash done")
    taskInfo.status = 2
    taskInfo.referencePaths = nodeRefs
    taskInfo.completeTime = new Date()
    if(taskInfo.referencePaths.length == 0){
      appDatabase.analysisTaskDao.delete(taskInfo)
        .then(()=>{
          return fileIo.unlink(taskInfo.heapSnapshotPath)
        })
        .catch(() => {
        hilog.error(0x0002, "Analyze", "delete taskInfo error")
      })
      LeakNotification.getInstance().publishNotification(file+" 暂未发现泄漏对象")
    }else {
      appDatabase.analysisTaskDao.update(taskInfo).catch(() => {
        hilog.error(0x0002, "Analyze", "update taskInfo error")
      })
      LeakNotification.getInstance().publishNotification(file+" 分析成功")
    }
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
  return rawAnalyzeHash(checkTask.task.heapSnapshotPath,checkTask.objInfos).then((nodeRefs)=>{
    hilog.debug(0x0002, "Analyze","analyzeHash done")
    taskInfo.status = 2
    taskInfo.referencePaths = nodeRefs
    taskInfo.completeTime = new Date()
    if(taskInfo.referencePaths.length == 0){
      appDatabase.analysisTaskDao.delete(taskInfo)
        .then(()=>{
          return fileIo.unlink(taskInfo.heapSnapshotPath)
        })
        .catch(() => {
          hilog.error(0x0002, "Analyze", "delete taskInfo error")
        })
      LeakNotification.getInstance().publishNotification(file+" 暂未发现泄漏对象")
    }else {
      appDatabase.analysisTaskDao.update(taskInfo).catch(() => {
        hilog.error(0x0002, "Analyze", "update taskInfo error")
      })
      LeakNotification.getInstance().publishNotification(file+" 分析成功")
    }
  }).catch(() => {
    hilog.error(0x0002, "Analyze", "analyzeHash error")
    taskInfo.status = 3
    appDatabase.analysisTaskDao.update(taskInfo).catch(() => {
      hilog.error(0x0002, "Analyze", "update taskInfo error")
    })
    LeakNotification.getInstance().publishNotification(file+" 分析失败")
  })
}