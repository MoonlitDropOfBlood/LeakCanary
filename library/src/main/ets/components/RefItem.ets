import { ReferenceChain } from "libleakguard.so"

@ComponentV2
export struct RefItem{

  @Param @Require item:RepeatItem<ReferenceChain>

  @Local bottomLineHeight:number = 0

  build() {
    ListItem(){
      Flex({ alignItems: ItemAlign.Stretch }) {
        Column(){
          Stack()
            .width(1)
            .height(14)
            .backgroundColor('#999999')
            .margin({left:2})
          Circle()
            .width(6)
            .height(6)
            .fill('#888888')
            .margin({left:2})
          if(this.item.item.from.name != 'SyntheticRoot'){
            Stack()
              .width(1)
              .height(this.bottomLineHeight)
              .backgroundColor('#999999')
              .margin({left:2})
          }
        }.onSizeChange((_,newValue)=>{
          this.bottomLineHeight = newValue.height as number - 20
        })



        Column({space:8}){
          Text(`引用类型:${this.item.item.edgeType}`)
          Text(`${this.item.item.from.name}@${this.item.item.from.nodeId.toString(16)}`)
          if(this.item.item.from.path){
            Text(`${this.item.item.from.path}(${this.item.item.from.line})`)
          }
        }.alignItems(HorizontalAlign.Start)
        .id('content')
        .height('auto')
        .margin({top:8,left:18})
      }.width('100%')
    }
    .padding({left:16,right:16})
  }
}