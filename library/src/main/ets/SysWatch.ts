import { jsLeakWatcher } from "@kit.PerformanceAnalysisKit"
import { HashSet, util } from "@kit.ArkTS"
import { LeakGuard } from "./LeakGuard"
import { ObjInfo } from "./model/ObjInfo"
import { appDatabase } from "./db/AppDatabase"
import { fileIo } from "@kit.CoreFileKit"
import { AnalysisTask } from "./db/DatabaseInterfaces"
import { CheckTask } from "./model/CheckTask"

export class SysWatch {
  
  private init: boolean = false
  
  private enabled: boolean = true
  
  private timerTask: number|undefined = undefined

  private cacheNoGC: HashSet<number> = new HashSet()
  
  registry(obj:object){
    if(!this.enabled){
      return
    }
    if(!this.init){
      this.init = true
      jsLeakWatcher.enable(true)
      this.timerTask = setInterval(()=>{
        let result = jsLeakWatcher.check()
        let leakInfos:ObjInfo[] = JSON.parse(result)
        result = undefined
        if(LeakGuard.isAutoClear()){
          const objInfos:ObjInfo[] = []
          leakInfos.forEach((info)=>{
            if(this.cacheNoGC.remove(info.hash))
              objInfos.push(info)
          })
          leakInfos = objInfos 
        }
        if(leakInfos.length == 0){
          return
        }
        this.dumpHeapSnapshot().then((taskInfo)=>{
          return this.analyzeHeapSnapshot({ task:taskInfo, objInfos:leakInfos }).then(()=>{
            return fileIo.unlink(taskInfo.heapSnapshotPath)
          })
        })
      },LeakGuard.getAnalyzeInterval())
    }
    jsLeakWatcher.watch(obj,util.getHash(obj).toString())
    if(LeakGuard.isAutoClear()) {
      this.cacheNoGC.add(util.getHash(obj))
    }
  }
  
  setEnabled(enabled: boolean){
    this.enabled = enabled
    jsLeakWatcher.enable(enabled)
    if(!enabled) {
      if (this.init) {
        this.init = false
        if (this.timerTask) {
          clearInterval(this.timerTask)
          this.timerTask = undefined
        }
        this.cacheNoGC.clear()
      }
    }
  }
  
  private dumpHeapSnapshot():Promise<AnalysisTask>{
    const baseDir = appDatabase.context.getApplicationContext().filesDir
    const paths = jsLeakWatcher.dump(baseDir)
    let taskInfo:AnalysisTask = {
      isViewed:false,
      status:1,
      createTime:new Date(),
      heapSnapshotPath:baseDir+'/'+paths[1],
      hashFile:baseDir+'/'+paths[0]
    }
    return appDatabase.analysisTaskDao.insert(taskInfo).then(()=>{
      return taskInfo
    })
  }

  analyzeHeapSnapshot:(task:CheckTask)=>Promise<void>
}

export const sysWatch = new SysWatch()
