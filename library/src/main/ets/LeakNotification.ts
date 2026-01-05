import { notificationManager } from "@kit.NotificationKit"

export class LeakNotification {
  //单例
  private static instance: LeakNotification = null
  public static getInstance(): LeakNotification {
    if (this.instance == null) {
      this.instance = new LeakNotification()
    }
    return this.instance
  }


  private constructor() {
    this.initPublisher()
  }

  private initPublisher() {
    if (!notificationManager.isNotificationEnabledSync()) {//通知权限未开启
      return
    }
    notificationManager.publish({
      content:{
        notificationContentType: notificationManager.ContentType.NOTIFICATION_CONTENT_BASIC_TEXT,
        normal:{
          title:"LeakCanary",
          text:"正在检测内存泄漏"
        }
      },
      label:"LeakCanary",
      notificationSlotType: notificationManager.SlotType.SERVICE_INFORMATION,
      isAlertOnce:true,
    })
  }

  publishNotification(size:number,firstName: string) {
    notificationManager.publish({
      content:{
        notificationContentType: notificationManager.ContentType.NOTIFICATION_CONTENT_BASIC_TEXT,
        normal:{
          title:"LeakCanary",
          text:`检测到${firstName}等${size}个组件泄漏`
        }
      },
      label:"LeakCanary",
      notificationSlotType: notificationManager.SlotType.SERVICE_INFORMATION,
      isAlertOnce:true,
    })
  }
}
