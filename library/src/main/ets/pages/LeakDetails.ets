import { RefItem } from "../components/RefItem";
import { AnalysisTask } from "../db/DatabaseInterfaces";
import { common } from "@kit.AbilityKit";
import { window } from "@kit.ArkUI";
import { NodeRef } from "libleakguard.so";

@Entry({routeName: 'LeakDetailsPage'})
@ComponentV2
export struct LeakDetails{

  private task:AnalysisTask = this.getUIContext().getRouter().getParams() as AnalysisTask

  @Local top:number = 0

  aboutToAppear(): void {
    (this.getUIContext().getHostContext() as common.UIAbilityContext).windowStage.getMainWindow().then((windowClass)=>{
      if(windowClass.getImmersiveModeEnabledState()){
        let top = windowClass.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM).topRect.height
        this.top = this.getUIContext().px2vp(top)
      }
    })
  }

  build() {
    Column(){
      Row({space:8}){
        Image($r('sys.media.ohos_ic_back'))
          .width(24)
          .height(24)
          .onClick(()=>{
            this.getUIContext().getRouter().back()
          })
          .fillColor('#999999')

        Text(this.task.heapSnapshotPath.replaceAll('.heapsnapshot','').replaceAll((this.getUIContext().getHostContext()?.getApplicationContext()?.filesDir ?? '')+'/','')+' '+(this.task.referencePaths?.length ?? 0)+'个泄漏')
          .fontSize(24)
          .fontWeight(FontWeight.Medium)
          .maxLines(1)
          .textOverflow({overflow:TextOverflow.Ellipsis})
          .ellipsisMode(EllipsisMode.CENTER)
          .layoutWeight(1)
      }.padding(16)
      .margin({top:this.top})
      .width('100%')

      List(){
        Repeat(this.task.referencePaths)
          .key((it)=>it.hash.toString())
          .virtualScroll({totalCount:this.task.referencePaths?.length ?? 0})
          .each((repeatItem:RepeatItem<NodeRef>)=>{
            ListItemGroup({header:this.itemHeader(repeatItem.item)}){
              Repeat(repeatItem.item.ref)
                .virtualScroll({totalCount:repeatItem.item.ref.length})
                .key((it)=>it.from.nodeId.toString(16))
                .each((item)=>{
                  RefItem({ item })
                })
            }.padding({bottom:8})
          })
      }
      .padding({left:16,right:16})
      .layoutWeight(1)
      .width('100%')
      .scrollBar(BarState.Off)
      .sticky(StickyStyle.Header)

    }.width('100%')
    .height('100%')
  }


  @Builder
  itemHeader(ref:NodeRef){
    Text(`${ref.name}@${ref.hash.toString(16)}`).width('100%')
      .padding(16)
      .borderRadius(8)
      .borderWidth(1)
      .borderColor('#80999999')
      .backgroundColor('#999999')
  }
}