import { appDatabase } from "../db/AppDatabase"
import { TaskVO } from "../model/TaskVO"
import { fileIo } from "@kit.CoreFileKit"
import { hilog } from "@kit.PerformanceAnalysisKit"

@ComponentV2
export struct TaskItem{

  @Param @Require repeatItem:RepeatItem<TaskVO>

  @Event onDelete:(vo:TaskVO)=>void

  build() {
    ListItem(){
      Row({space:16}){
        Text(){
          Span(this.repeatItem.item.task.referencePaths?.length.toString() ?? '0')
        }
        .fontSize(20)
        .fontWeight(FontWeight.Bold)
        .height(48)
        .width(48)
        .align(Alignment.Center)
        .textAlign(TextAlign.Center)
        .borderRadius(8)
        .fontColor('#ffe99100')
        .backgroundColor('#30003688')

        Column({space:8}){
          Text(this.repeatItem.item.task.heapSnapshotPath)
            .fontSize(16)

          Text(`${this.getStatus(this.repeatItem.item.task.status)} ${this.repeatItem.item.task.completeTime?.toLocaleString() ?? ''} ${this.repeatItem.item.isViewed ? '已查看' : '未查看'}`)
            .fontSize(14)
        }.layoutWeight(1)
        .alignItems(HorizontalAlign.Start)
      }.padding(16)
      .backgroundColor('#80999999')
      .borderRadius(8)
      .onClick(()=>{
        this.repeatItem.item.isViewed = true
        this.repeatItem.item.task.isViewed = true
        appDatabase.analysisTaskDao.update(this.repeatItem.item.task).catch(() => {
          hilog.error(0x0002, "LeakPage", "update taskInfo error")
        })
        this.getUIContext().getRouter().pushNamedRoute({
          name:'LeakDetailsPage',
          params: this.repeatItem.item.task
        }).catch(() => {
          hilog.error(0x0002, "LeakPage", "pushNamedRoute error")
        })
      })
    }.padding({left:16,right:16})
    .swipeAction({end:{onAction:()=>{
      appDatabase.analysisTaskDao.delete(this.repeatItem.item.task)
        .then(()=>{
          this.onDelete(this.repeatItem.item)
          fileIo.unlink(this.repeatItem.item.task.heapSnapshotPath)
        })
        .catch(() => {
          hilog.error(0x0002, "LeakPage", "update taskInfo error")
        })
    }},edgeEffect:SwipeEdgeEffect.Spring})
  }

  getStatus(status:number):string{
    switch (status) {
      case 1:
        return '分析中'
      case 2:
        return '已完成'
      case 3:
        return '分析失败'
      default:
        return '未知'
    }
  }
}