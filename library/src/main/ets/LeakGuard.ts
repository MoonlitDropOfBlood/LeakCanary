import { UIContext, uiObserver } from "@kit.ArkUI"
import { getOpenHarmonyInternalApi } from "./Handler"
import { objWatch } from "./ObjWatch"
import { deviceInfo } from "@kit.BasicServicesKit"
import hilog from "@ohos.hilog"
import { LeakNotification } from "./LeakNotification"
import { appDatabase } from "./db/AppDatabase"
import { common } from "@kit.AbilityKit"
import { sysWatch } from "./SysWatch"
import { autoWatch } from "./AutoWatch"

export enum WatchLevel {

  /**
   * 基础监听
   */
  BASE = 0,
  /**
   * 开启系统监听
   */
  SYS = 1,
  /**
   * 开启API20及以上版本对象监听
   */
  API20 = 2,

}

export class LeakGuard {

  private static isInit: boolean = false


  /**
   * 分析间隔
   */
  private static interval:number = 30 * 1000

  private static autoClear:boolean = false

  private static enabledWatchLevel: WatchLevel = WatchLevel.API20


  /**
   * 设置是否开启系统监听
   * @param enabledSysWatch 是否开启系统监听
   * @api
   */
  static setWatchLevel(enabledWatchLevel: WatchLevel) {
    LeakGuard.enabledWatchLevel = enabledWatchLevel
  }

  static setEnabled(enabled: boolean) {
    if(!enabled){
      sysWatch.setEnabled(enabled)
      objWatch.setEnabled(enabled)
      autoWatch.setEnabled(enabled)
    }else{
      switch (LeakGuard.enabledWatchLevel) {
        case WatchLevel.BASE:
          objWatch.setEnabled(enabled)
          break
        case WatchLevel.SYS:
          sysWatch.setEnabled(enabled)
          break
        case WatchLevel.API20:
          autoWatch.setEnabled(enabled)
          break
      }
    }
  }

  /**
   * 初始化全局自定义组件监听
   * 支持最新的自动监听方式
   * 全新的监听方式
   * @since 20
   * @api
   */
  static initRegisterGlobalWatch(context: common.UIAbilityContext){
    if(deviceInfo.sdkApiVersion < 20){
      hilog.warn(0x0001, "LeakGuard", "initRegisterGlobalWatch only support sdkApiVersion >= 20")
      return
    }
    if(LeakGuard.isInit){
      hilog.warn(0x0001, "LeakGuard", "initRegisterGlobalWatch already init")
      return
    }
    appDatabase.init(context)
    LeakNotification.getInstance().initPublisher(context)
    if(this.enabledWatchLevel == WatchLevel.API20){
      autoWatch.setEnabled(true)
      return
    }
    try {
      let openHarmonyInternalApi = getOpenHarmonyInternalApi()
      openHarmonyInternalApi((owner:WeakRef<object>,msg:string)=>{
        const component = owner.deref()
        if(component) {
          const name = component.constructor.name
          if(name == 'LeakPage' || name == 'LeakDetails' || name == 'TaskItem' || name == 'RefItem'){
            return
          }
          LeakGuard.watchObj(component)
        }
      })
      LeakGuard.isInit = true
    }catch (e) {
      hilog.error(0x0001, "LeakGuard", "initRegisterGlobalWatch error " + e)
    }
  }

  /**
   * 监听对象
   * @param obj
   * @api
   */
  static watchObj(obj:object){
    if(LeakGuard.enabledWatchLevel >= WatchLevel.SYS){
      sysWatch.registry(obj)
    }else {
      objWatch.registry(obj)
    }
  }

  /**
   * 设置是否自动清除
   * @param autoClear 是否自动清除
   */
  static setAutoClear(autoClear:boolean){
    LeakGuard.autoClear = autoClear
  }

  static isAutoClear():boolean{
    return LeakGuard.autoClear
  }

  /**
   * 设置分析间隔
   * @param timer 分析间隔 单位秒
   * @api
   */
  static setAnalyzeInterval(interval:number){
    LeakGuard.interval = interval * 1000
  }

  /**
   * 获取分析间隔
   * @returns 分析间隔 单位秒
   * @api
   */
  static getAnalyzeInterval():number{
    return LeakGuard.interval
  }

  /**
   * 注册根页面组件监听
   * @param rootComponent
   * @since 12
   * @api
   */
  static registerRootWatch(rootComponent: object) {
    LeakGuard.registerComponent(rootComponent)
    const uiContext:UIContext = rootComponent['getUIContext']()
    appDatabase.init(uiContext.getHostContext()!!.getApplicationContext())
    uiContext.getUIObserver().on("navDestinationUpdate", (navInfo) => {
      if (navInfo.state == uiObserver.NavDestinationState.ON_WILL_DISAPPEAR && navInfo.uniqueId) {
        let map: Map<number, WeakRef<object>> = rootComponent["childrenWeakrefMap_"]
        map.forEach((value, key) => {
          if (key + 1 == navInfo.uniqueId) {
            let pageComponent = value.deref()
            LeakGuard.registerAllChild(pageComponent)
          }
        })
      }
    })
    LeakNotification.getInstance().initPublisher(uiContext.getHostContext()!! as common.UIAbilityContext)
  }

  private static registerAllChild(pageComponent: object) {
    LeakGuard.watchObj(pageComponent)
    let map: Map<number, WeakRef<object>> = pageComponent["childrenWeakrefMap_"]
    if (map.size == 0) {
      return
    }
    map.forEach((value) => {
      let childComponent = value.deref()
      LeakGuard.registerAllChild(childComponent)
    });
  }

  /**
   * 注册单个组件监听
   * @param component 自定义组件
   * @since 12
   * @api
   */
  static registerComponent(component: object){
    let uniqueId: number = component['getUniqueId']();
    let uiContext:UIContext = component['getUIContext']()
    uiContext.postFrameCallback({
      onFrame:(frameTimeInNano:number)=>{
        let node = uiContext.getFrameNodeByUniqueId(uniqueId)
        node.commonEvent.setOnDisappear(()=>{
          LeakGuard.registerAllChild(component)
        })
      },
      onIdle:(timeLeftInNano: number)=>{}
    })

  }
}