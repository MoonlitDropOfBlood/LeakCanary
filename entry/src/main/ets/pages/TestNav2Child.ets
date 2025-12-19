import { ZRoute, ZRouter } from "@hzw/zrouter"
import { a } from "../A"
import { Custom1Component } from "./CustomComponent"

@ZRoute({ name: "entry/TestNav2Child",useV2:true,useTemplate:true})
@ComponentV2
export struct TestNav2Child {

  a:number[] = [1,2,3]

  aboutToAppear(): void {
    a.push(this)
  }

  onDidBuild(): void {
    console.log('did build')
  }

  build() {
    Column() {
      Text('测试')
      ForEach(this.a, (item:number) => {
        Custom1Component()
          .backgroundColor(Color.White)
          .onClick(() => {
            ZRouter.getInstance().push("entry/TestNav1Child")
          })
      })
    }
  }
}