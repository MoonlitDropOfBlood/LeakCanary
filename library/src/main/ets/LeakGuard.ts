import { UIContext, uiObserver } from "@kit.ArkUI"
import { getOpenHarmonyInternalApi } from "./Handler"
import { objWatch } from "./ObjWatch"
import { deviceInfo } from "@kit.BasicServicesKit"
import hilog from "@ohos.hilog"
import { LeakNotification } from "./LeakNotification"
import { appDatabase } from "./db/AppDatabase"
import { common } from "@kit.AbilityKit"

export class LeakGuard {

  private static isInit: boolean = false

  /**
   * 初始化全局自定义组件监听
   * 全新的监听方式
   * @since 20
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
    try {
      let openHarmonyInternalApi = getOpenHarmonyInternalApi()
      openHarmonyInternalApi((owner:WeakRef<object>,msg:string)=>{
        const component = owner.deref()
        if(component) {
          objWatch.registry(component)
        }
      })
      LeakGuard.isInit = true
      LeakNotification.getInstance().initPublisher(context)
    }catch (e) {
      hilog.error(0x0001, "LeakGuard", "initRegisterGlobalWatch error " + e)
    }
  }

  /**
   * 注册根页面组件监听
   * @param rootComponent
   * @since 12
   */
  static registerRootWatch(rootComponent: object) {
    LeakGuard.registerComponent(rootComponent)
    const uiContext:UIContext = rootComponent['getUIContext']()
    appDatabase.init(uiContext.getHostContext()!!)
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
    objWatch.registry(pageComponent)
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