import { appDatabase } from "../db/AppDatabase"
import { hilog } from "@kit.PerformanceAnalysisKit"
import { TaskVO } from "../model/TaskVO"
import './LeakDetails'
import { LEAK_TASK_ROUTE_NAME } from "../Constants"

@Preview
@Entry({routeName: LEAK_TASK_ROUTE_NAME})
@ComponentV2
export struct LeakPage {

  @Local leakTasks:TaskVO[] = []

  aboutToAppear(): void {
    appDatabase.analysisTaskDao.query().then((tasks)=>{
      this.leakTasks = tasks.map((item)=>new TaskVO(item))
    })
  }

  build() {
    Column(){
      Text(`${this.leakTasks.length} 次泄漏分析`)
        .fontSize(24)
        .margin(16)
        .fontWeight(FontWeight.Medium)

      List(){
        Repeat(this.leakTasks)
          .key((item)=>item.taskId?.toString() ?? '')
          .virtualScroll({totalCount:this.leakTasks.length})
          .each((repeatItem:RepeatItem<TaskVO>)=>{
            ListItem(){
              Row({space:16}){
                Text(){
                  Span(repeatItem.item.task.referencePaths?.toString() ?? '0')
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
                  Text(repeatItem.item.task.heapSnapshotPath)
                    .fontSize(16)

                  Text(`${this.getStatus(repeatItem.item.task.status)} ${repeatItem.item.task.completeTime?.toLocaleString() ?? ''} ${repeatItem.item.isViewed ? '已查看' : '未查看'}`)
                    .fontSize(14)
                }.layoutWeight(1)
              }.padding(16)
              .backgroundColor('#80999999')
              .borderRadius(8)
              .onClick(()=>{
                repeatItem.item.isViewed = true
                repeatItem.item.task.isViewed = true
                appDatabase.analysisTaskDao.update(repeatItem.item.task).catch(() => {
                  hilog.error(0x0002, "LeakPage", "update taskInfo error")
                })
                this.getUIContext().getRouter().pushNamedRoute({
                  name:'LeakDetailsPage',
                  params:repeatItem.item.task
                }).catch(() => {
                  hilog.error(0x0002, "LeakPage", "pushNamedRoute error")
                })
              })
            }.padding({left:16,right:16})
          })
      }.layoutWeight(1)
      .width('100%')
      .divider({
        color:Color.Transparent,
        strokeWidth:16
      })
    }.width('100%')
    .height('100%')
    .alignItems(HorizontalAlign.Start)
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
