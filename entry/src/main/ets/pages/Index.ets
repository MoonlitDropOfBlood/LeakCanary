import { ZRouter } from '@hzw/zrouter';
import { Custom1Component } from './CustomComponent';
import { notificationManager } from '@kit.NotificationKit';
import { LEAK_TASK_ROUTE_NAME } from '@duke/leak-guard';

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  aboutToAppear(): void {
    if(!notificationManager.isNotificationEnabledSync()){
      notificationManager.requestEnableNotification()
    }
  }

  build() {
    Navigation(ZRouter.getNavStack()) {
      RelativeContainer() {
        Custom1Component()
          .backgroundColor(Color.Red)
          .height(200)
          .width(200)
        Text(this.message)
          .id('HelloWorld')
          .fontSize($r('app.float.page_text_font_size'))
          .fontWeight(FontWeight.Bold)
          .alignRules({
            center: { anchor: '__container__', align: VerticalAlign.Center },
            middle: { anchor: '__container__', align: HorizontalAlign.Center }
          })
          .onClick(() => {
            ZRouter.getInstance().push("entry/TestNav1Child")
          })

        Button('调试分析页面')
          .onClick(()=>{
            this.getUIContext().getRouter().pushNamedRoute({name:LEAK_TASK_ROUTE_NAME})
          })
          .alignRules({
            top: { anchor: '__container__', align: VerticalAlign.Top },
            right: { anchor: '__container__', align: HorizontalAlign.End }
          })

      }
      .height('100%')
      .width('100%')
    }
  }
}