import { jsLeakWatcher } from "@kit.PerformanceAnalysisKit"
import { util } from "@kit.ArkTS"
import { fileIo } from "@kit.CoreFileKit"
import { LeakInfo, ObjInfo } from "./model/ObjInfo"
import { LeakNotification } from "./LeakNotification"
import { AnalysisTask } from "./db/DatabaseInterfaces"
import { appDatabase } from "./db/AppDatabase"
import { CheckTask } from "./model/CheckTask"

export class AutoWatch {

  register(obj:object){
    jsLeakWatcher.watch(obj,util.getHash(obj).toString())
  }

  setEnabled(enabled: boolean) {
    if(!enabled) {
      jsLeakWatcher.enable(enabled)
    }else{
      jsLeakWatcher.enableLeakWatcher(true,[],(path:string[])=>{
        this.getLeakInfo(path[0].replace("//","/"),path[1].replace("//","/"))
      })
    }
  }

  async getLeakInfo(leakInfoPath:string,rawPath:string){
    let leakInfoStr:string|undefined = await fileIo.readText(leakInfoPath, "utf-8")
    const leakInfo = JSON.parse(leakInfoStr) as LeakInfo
    if(leakInfo.leakObjList.length == 0){
      return
    }
    leakInfoStr = undefined
    let array:Array<ObjInfo> = leakInfo.leakObjList
    LeakNotification.getInstance().publishNotification(`检测到${array[0].name}等${array.length}个组件疑似泄漏`)
    let taskInfo:AnalysisTask = {
      isViewed:false,
      status:1,
      createTime:new Date(),
      heapSnapshotPath:rawPath
    }
    return appDatabase.analysisTaskDao.insert(taskInfo).then(()=>{
      return fileIo.unlink(leakInfoPath)
    }).then(()=>{
      this.getDumpInfo({ task:taskInfo, objInfos:array })
    })
  }

  getDumpInfo: (task:CheckTask)=>Promise<void>
}

export const autoWatch = new AutoWatch()
