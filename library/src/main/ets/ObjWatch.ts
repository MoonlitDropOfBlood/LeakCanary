import { LinkedList, util } from '@kit.ArkTS'
import { hilog } from '@kit.PerformanceAnalysisKit'

interface ComponentObjInfo{
  name:String
  hash:number
}


class ObjWatch {
  cache:WeakMap<object,ComponentObjInfo> = new WeakMap()
  cacheValue:LinkedList<WeakRef<ComponentObjInfo>> = new LinkedList()

  targetGC:WeakRef<object>|undefined

  registry(owner: object) {
    let info:ComponentObjInfo = {
      name: owner.constructor.name,
      hash: util.getHash(owner)
    }
    this.cache.set(owner, info)
    this.cacheValue.add(new WeakRef(info))
    if (this.targetGC?.deref() == undefined) {
      let gcSource = new Object()
      this.targetGC = new WeakRef(gcSource)
      const registry = new FinalizationRegistry((heldValue: ObjWatch) => {
        heldValue.cacheValue.forEach((it: WeakRef<ComponentObjInfo>) => {
          let info = it.deref()
          if (info == undefined) {
            heldValue.cacheValue.remove(it)
          }else{
            hilog.error(0x0001,"GC",  `组件 ${info.name} 可能发生泄漏，hash值为${info.hash}`)
          }
        })
        hilog.error(0x0001,"GC","可能泄漏的组件为" + heldValue.cacheValue.length)
      });
      registry.register(gcSource, this)
    }
  }
}

export const objWatch = new ObjWatch()