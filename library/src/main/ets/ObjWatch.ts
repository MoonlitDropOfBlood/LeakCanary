import { HashSet, LinkedList, util } from '@kit.ArkTS'
import { hilog } from '@kit.PerformanceAnalysisKit'
import { LeakNotification } from './LeakNotification'
import { common } from '@kit.AbilityKit'
import { systemDateTime } from '@kit.BasicServicesKit'

export enum Sensitivity{
  /**
   * 高灵敏度
   * 容易产生误报，但是能够及时发现短时内存泄漏
   */
  HEIGHT = 1,
  /**
   * 低灵敏度
   * 基本不会产生误报，但是触发时间比较久
   * 默认值
   */
  LOW = 2
}

class ObjWatch {
  private cacheValue:LinkedList<WeakRef<object>> = new LinkedList()
  private cacheGCCount:WeakMap<object, number> = new WeakMap()

  private targetGC:WeakRef<object>|undefined

  private context:common.ApplicationContext

  private sensitivity = Sensitivity.LOW

  private autoClear:boolean = false

  private isAnalyzing:boolean = false

  private lastAnalyzeTime:number = 0

  /**
   * 分析间隔
   */
  private interval:number = 30 * 1000

  /**
   * 设置灵敏度
   * @param sensitivity 灵敏度
   */
  setSensitivity(sensitivity:Sensitivity){
    this.sensitivity = sensitivity
  }

  /**
   * 设置是否自动清除
   * @param autoClear 是否自动清除
   */
  setAutoClear(autoClear:boolean){
    this.autoClear = autoClear
  }

  /**
   * 设置分析间隔
   * @param timer 分析间隔 单位秒
   */
  setAnalyzeInterval(interval:number){
    this.interval = interval * 1000
  }

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
            if(systemDateTime.getTime() - heldValue.lastAnalyzeTime >= heldValue.interval) {
              heldValue.lastAnalyzeTime = systemDateTime.getTime()
              const cloneCache = heldValue.cacheValue.clone()
              heldValue.isAnalyzing = true
              heldValue.analyzeHeapSnapshot(noGC).then(() => {
                if (this.autoClear) {
                  cloneCache.forEach((cloneItem) => {
                    heldValue.cacheValue.remove(cloneItem)
                  })
                }
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
  analyzeHeapSnapshot:(objects:HashSet<object>)=>Promise<void>
}

export const objWatch = new ObjWatch()