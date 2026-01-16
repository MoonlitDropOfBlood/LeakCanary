import { HashSet, LinkedList, util } from '@kit.ArkTS'
import { hidebug, hilog } from '@kit.PerformanceAnalysisKit'
import { LeakNotification } from './LeakNotification'
import { common } from '@kit.AbilityKit'
import { systemDateTime } from '@kit.BasicServicesKit'
import { LeakGuard } from './LeakGuard'
import { AnalysisTask } from './db/DatabaseInterfaces'
import { appDatabase } from './db/AppDatabase'
import { ObjInfo } from './model/ObjInfo'
import { CheckTask } from './model/CheckTask'

class ObjWatch {
  private cacheValue:LinkedList<WeakRef<object>> = new LinkedList()
  private cacheGCCount:WeakMap<object, number> = new WeakMap()

  private targetGC:WeakRef<object>|undefined

  private context:common.ApplicationContext

  private sensitivity = 2

  private isAnalyzing:boolean = false

  private lastAnalyzeTime:number = 0

  private enabled:boolean = true

  registry(owner: object) {
    if(!this.enabled){
      return
    }
    if(!this.context){
      this.context = owner['getUIContext']().getHostContext().getApplicationContext()
    }
    this.cacheValue.add(new WeakRef(owner))
    if (this.targetGC?.deref() == undefined) {
      let gcSource = new Object()
      this.targetGC = new WeakRef(gcSource)
      const registry = new FinalizationRegistry((heldValue: ObjWatch) => {
        let noGC = new HashSet<object>()
        let firstLeak: string = undefined
        heldValue.cacheValue.forEach((it: WeakRef<object>) => {
          let info = it.deref()
          if (info == undefined) {
            heldValue.cacheValue.remove(it)
          }else{
            let oldCount = (heldValue.cacheGCCount.get(info) ?? 0) + 1
            heldValue.cacheGCCount.set(info, oldCount)
            if(oldCount >= heldValue.sensitivity) {
              if (noGC.add(info)) {
                hilog.error(0x0001, "GC", `对象 ${info.constructor.name} 可能发生泄漏，hash值为${util.getHash(info)}`)
                if (firstLeak == undefined) {
                  firstLeak = info.constructor.name
                }
              }
            }
          }
        })
        if(noGC.length > 0) {
          hilog.error(0x0001,"GC","可能泄漏的对象为数为 " + noGC.length)
          LeakNotification.getInstance().publishNotification(`检测到${firstLeak}等${noGC.length}个组件泄漏`)
          if(heldValue.isAnalyzing == false) {
            if(systemDateTime.getTime() - heldValue.lastAnalyzeTime >= LeakGuard.getAnalyzeInterval()) {
              heldValue.lastAnalyzeTime = systemDateTime.getTime()
              const cloneCache = heldValue.cacheValue.clone()
              heldValue.isAnalyzing = true
              noGC.forEach((it) => {// 清除已分析对象的GC计数，降低dump压力，避免重复分析
                heldValue.cacheGCCount.delete(it)
              })
              heldValue.dumpHeapSnapshot(noGC)
                .then((task) => {
                 return heldValue.analyzeHeapSnapshot(task)
                }).then(()=>{
                  if (LeakGuard.isAutoClear()) {
                    cloneCache.forEach((cloneItem) => {
                      heldValue.cacheValue.remove(cloneItem)
                    })
                  }
                }).catch(()=>{
                  heldValue.isAnalyzing = false
                })
            }
          }else{
            noGC.clear()
          }
        }
      });
      registry.register(gcSource, this)
    }
  }

  private dumpHeapSnapshot(objects:HashSet<object>):Promise<CheckTask>{
    const time = systemDateTime.getTime()
    const file = time+"-泄漏"
    let taskInfo:AnalysisTask = {
      isViewed:false,
      status:1,
      createTime:new Date(time),
      heapSnapshotPath:appDatabase.context.filesDir+'/'+file+'.heapsnapshot'
    }
    const array :ObjInfo[] = []
    objects.forEach((it:object)=>{
      array.push({
        hash:util.getHash(it),
        name:it.constructor.name
      })
    })
    objects.clear() // 防止 GC 出现意外的引用
    return appDatabase.analysisTaskDao.insert(taskInfo).then(()=>{
      hidebug.dumpJsHeapData(taskInfo.heapSnapshotPath)
      return {
        task:taskInfo,
        objInfos:array
      }
    })
  }

  setEnabled(enabled: boolean) {
    if(!enabled){
      this.cacheValue.clear()
      this.targetGC = undefined
    }
    this.enabled = enabled
  }

  analyzeHeapSnapshot:(task:CheckTask)=>Promise<void>
}

export const objWatch = new ObjWatch()