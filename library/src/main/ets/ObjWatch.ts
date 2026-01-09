import { HashSet, LinkedList, util } from '@kit.ArkTS'
import { hilog } from '@kit.PerformanceAnalysisKit'
import { LeakNotification } from './LeakNotification'
import { common } from '@kit.AbilityKit'
import { NodeInfo } from './model/NodeInfo'

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
          let nodeInfos = Array.from(noGC.values()).map(it=>({
            hash:util.getHash(it),
            name:it.constructor.name
          }))
          noGC.clear()// 清空noGC，防止后续分析中出现不必要的引用
          heldValue.analyzeHeapSnapshot(nodeInfos)
        }
      });
      registry.register(gcSource, this)
    }
  }
  analyzeHeapSnapshot:(objects:NodeInfo[])=>void
}

export const objWatch = new ObjWatch()