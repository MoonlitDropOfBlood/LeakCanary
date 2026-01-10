import { ReferenceChain } from "libleakguard.so"

@ComponentV2
export struct RefItem{

  @Param @Require item:RepeatItem<ReferenceChain>

  build() {
    ListItem(){
      Flex({ alignItems: ItemAlign.Stretch }) {
        Column(){
          Stack()
            .width(2)
            .height(14)
            .backgroundColor('#999999')

          Circle()
            .width(8)
            .height(8)
            .fill('#888888')
        }.backgroundColor(this.item.item.from.name != 'SyntheticRoot' ? '#999999' : Color.Transparent)
        .width(2)
        .alignItems(HorizontalAlign.Center)


        Column({space:8}){
          Text(`引用类型:${this.item.item.edgeType}`)
          Text(`${this.item.item.from.name}@${this.item.item.from.nodeId.toString(16)}`)
          if(this.item.item.from.path){
            Text(`${this.item.item.from.path}(${this.item.item.from.line})`)
          }
        }.alignItems(HorizontalAlign.Start)
        .id('content')
        .height('auto')
        .padding({top:8,left:18})
      }.width('100%')
    }
    .padding({left:16,right:16})
  }
}