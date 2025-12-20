import { ZRouter } from '@hzw/zrouter';
import { Custom1Component } from './CustomComponent';
import { LeakCanary } from '@duke/leak-canary'

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  aboutToAppear(): void {
    LeakCanary.registerRootWatch(this)
  }

  build() {
    Navigation(ZRouter.getNavStack()) {
      RelativeContainer() {
        Custom1Component()
          .backgroundColor(Color.White)
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
      }
      .height('100%')
      .width('100%')
    }
  }
}