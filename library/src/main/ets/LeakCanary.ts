import { UIContext, uiObserver } from "@kit.ArkUI"
import { getOpenHarmonyInternalApi } from "./Handler"
import { objWatch } from "./ObjWatch"
import { deviceInfo } from "@kit.BasicServicesKit"
import hilog from "@ohos.hilog"
import { LeakNotification } from "./LeakNotification"

export class LeakCanary {

  private static isInit: boolean = false

  /**
   * 初始化全局自定义组件监听
   * 全新的监听方式
   * @since 20
   */
  static initRegisterGlobalWatch(){
    if(deviceInfo.sdkApiVersion < 20){
      hilog.warn(0x0001, "LeakCanary", "initRegisterGlobalWatch only support sdkApiVersion >= 20")
      return
    }
    if(LeakCanary.isInit){
      hilog.warn(0x0001, "LeakCanary", "initRegisterGlobalWatch already init")
      return
    }
    try {
      let openHarmonyInternalApi = getOpenHarmonyInternalApi()
      openHarmonyInternalApi((owner:WeakRef<object>,msg:string)=>{
        const component = owner.deref()
        if(component) {
          objWatch.registry(component)
        }
      })
      LeakCanary.isInit = true
      LeakNotification.getInstance()
    }catch (e) {
      hilog.error(0x0001, "LeakCanary", "initRegisterGlobalWatch error " + e)
    }
  }

  /**
   * 注册根页面组件监听
   * @param rootComponent
   * @since 12
   */
  static registerRootWatch(rootComponent: object) {
    LeakCanary.registerComponent(rootComponent)
    rootComponent['getUIContext']().getUIObserver().on("navDestinationUpdate", (navInfo) => {
      if (navInfo.state == uiObserver.NavDestinationState.ON_WILL_DISAPPEAR && navInfo.uniqueId) {
        let map: Map<number, WeakRef<object>> = rootComponent["childrenWeakrefMap_"]
        map.forEach((value, key) => {
          if (key + 1 == navInfo.uniqueId) {
            let pageComponent = value.deref()
            LeakCanary.registerAllChild(pageComponent)
          }
        })
      }
    })
    LeakNotification.getInstance()
  }

  private static registerAllChild(pageComponent: object) {
    objWatch.registry(pageComponent)
    let map: Map<number, WeakRef<object>> = pageComponent["childrenWeakrefMap_"]
    if (map.size == 0) {
      return
    }
    map.forEach((value) => {
      let childComponent = value.deref()
      LeakCanary.registerAllChild(childComponent)
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
          LeakCanary.registerAllChild(component)
        })
      },
      onIdle:(timeLeftInNano: number)=>{}
    })

  }
}