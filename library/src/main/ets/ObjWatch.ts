import { HashSet, LinkedList, util } from '@kit.ArkTS'
import { hilog } from '@kit.PerformanceAnalysisKit'

class ObjWatch {
  cacheValue:LinkedList<WeakRef<object>> = new LinkedList()

  private targetGC:WeakRef<object>|undefined

  registry(owner: object) {
    this.cacheValue.add(new WeakRef(owner))
    if (this.targetGC?.deref() == undefined) {
      let gcSource = new Object()
      this.targetGC = new WeakRef(gcSource)
      const registry = new FinalizationRegistry((heldValue: ObjWatch) => {
        let noGC = new HashSet<object>()
        heldValue.cacheValue.forEach((it: WeakRef<object>) => {
          let info = it.deref()
          if (info == undefined) {
            heldValue.cacheValue.remove(it)
          }else{
            if(noGC.add(info)) {
              hilog.error(0x0001, "GC", `组件 ${owner.constructor.name} 可能发生泄漏，hash值为${util.getHash(info)}`)
            }
          }
        })
        hilog.error(0x0001,"GC","可能泄漏的组件为数为 " + noGC.length)
      });
      registry.register(gcSource, this)
    }
  }
}

export const objWatch = new ObjWatch()