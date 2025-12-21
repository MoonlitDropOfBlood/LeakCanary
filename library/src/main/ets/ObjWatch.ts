import { LinkedList, util } from '@kit.ArkTS'
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
        let classNames:string[] = []
        heldValue.cacheValue.forEach((it: WeakRef<object>) => {
          let info = it.deref()
          if (info == undefined) {
            heldValue.cacheValue.remove(it)
          }else{
            hilog.error(0x0001,"GC",  `组件 `)
            classNames.push(info['extraInfo_']['page'])
          }
        })
        if(heldValue.cacheValue.length > 0) {
          hilog.error(0x0001, "GC", "可能泄漏的组件为" + heldValue.cacheValue.length)
          hidebug.dumpJsHeapData("heapData")
          heldValue.analyzeHeapSnapshot(heldValue.context.filesDir+"/heapData.heapsnapshot", classNames)
        }
      });
      registry.register(gcSource, this)
    }
  }
  analyzeHeapSnapshot:(file:string,className:string[])=>void
}

export const objWatch = new ObjWatch()