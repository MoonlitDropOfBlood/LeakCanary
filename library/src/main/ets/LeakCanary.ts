import { uiObserver } from "@kit.ArkUI"
import { objWatch } from "./ObjWatch"

export class LeakCanary {

  static registerRootWatch(rootComponent: object) {
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
  }

  static registerAllChild(pageComponent: object) {
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
}