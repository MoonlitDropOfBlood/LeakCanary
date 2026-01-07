import { HashSet, LinkedList, util } from '@kit.ArkTS'
import { hidebug, hilog } from '@kit.PerformanceAnalysisKit'
import { LeakNotification } from './LeakNotification'
import { systemDateTime } from '@kit.BasicServicesKit'
import { common } from '@kit.AbilityKit'

class ObjWatch {
  private cacheValue:LinkedList<WeakRef<object>> = new LinkedList()
  private cacheGCCount:WeakMap<object, number> = new WeakMap()

  private targetGC:WeakRef<object>|undefined

  private context:common.ApplicationContext


  registry(owner: object) {
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
            if(oldCount >= 2) {
              if (noGC.add(info)) {
                hilog.error(0x0001, "GC", `组件 ${info.constructor.name} 可能发生泄漏，hash值为${util.getHash(info)}`)
                if (firstLeak == undefined) {
                  firstLeak = info.constructor.name
                }
              }
            }
          }
        })
        if(noGC.length > 0) {
          hilog.error(0x0001,"GC","可能泄漏的组件为数为 " + noGC.length)
          LeakNotification.getInstance().publishNotification(noGC.length, firstLeak)
          const file = systemDateTime.getTime()+"-泄漏"
          hidebug.dumpJsHeapData(file)
          heldValue.analyzeHeapSnapshot(heldValue.context.filesDir+"/"+file+".heapsnapshot",  Array.from(noGC.values()))
        }
      });
      registry.register(gcSource, this)
    }
  }
  analyzeHeapSnapshot:(file:string,objects:object[])=>void
}

export const objWatch = new ObjWatch()