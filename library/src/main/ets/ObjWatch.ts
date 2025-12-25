import { HashSet, LinkedList, util } from '@kit.ArkTS'
import { hidebug, hilog } from '@kit.PerformanceAnalysisKit'
import { Context } from '@kit.AbilityKit'


class ObjWatch {
  cacheValue:LinkedList<WeakRef<object>> = new LinkedList()

  targetGC:WeakRef<object>|undefined

  context:Context


  registry(owner: object) {
    if(!this.context){
      this.context = owner['getUIContext']().getHostContext().getApplicationContext()
    }
    this.cacheValue.add(new WeakRef(owner))
    if (this.targetGC?.deref() == undefined) {
      let gcSource = new Object()
      this.targetGC = new WeakRef(gcSource)
      const registry = new FinalizationRegistry((heldValue: ObjWatch) => {
        let objects:HashSet<object> = new HashSet()
        heldValue.cacheValue.forEach((it: WeakRef<object>) => {
          let info = it.deref()
          if (info == undefined) {
            heldValue.cacheValue.remove(it)
          }else{
            objects.add(info)
          }
        })
        if(objects.length > 0) {
          hilog.error(0x0001, "GC", "可能泄漏的组件为" + objects.length)
          hidebug.dumpJsHeapData("heapData")
          heldValue.analyzeHeapSnapshot(heldValue.context.filesDir+"/heapData.heapsnapshot",  Array.from(objects.values()))
        }
      });
      registry.register(gcSource, this)
    }
  }
  analyzeHeapSnapshot:(file:string,objects:object[])=>void
}

export const objWatch = new ObjWatch()