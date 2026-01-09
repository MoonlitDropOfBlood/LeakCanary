import { appDatabase } from "../db/AppDatabase"
import { TaskVO } from "../model/TaskVO"
import './LeakDetails'
import { LEAK_TASK_ROUTE_NAME } from "../Constants"
import { analysisTaskTable } from "../db/DatabaseInterfaces"
import { TaskItem } from "../components/TaskItem"

@Preview
@Entry({routeName: LEAK_TASK_ROUTE_NAME})
@ComponentV2
export struct LeakPage {

  @Local leakTasks:TaskVO[] = []

  aboutToAppear(): void {
    appDatabase.analysisTaskDao.query(it=>it.orderByDesc(analysisTaskTable.taskId)).then((tasks)=>{
      this.leakTasks = tasks.map((item)=>new TaskVO(item))
    })
  }

  build() {
    Column(){
      Row(){
        Text('LeakGuard')
          .fontSize(24)
          .fontWeight(FontWeight.Medium)

        Text(`${this.leakTasks.length} 次泄漏分析`)
          .fontSize(24)
          .fontWeight(FontWeight.Medium)
      }.padding(16)
      .width('100%')
     .justifyContent(FlexAlign.SpaceBetween)

      List(){
        Repeat(this.leakTasks)
          .key((item)=>item.taskId?.toString() ?? '')
          .virtualScroll({totalCount:this.leakTasks.length})
          .each((repeatItem:RepeatItem<TaskVO>)=>{
            TaskItem({repeatItem,onDelete:(vo)=>{
              //从数组中删除被删除的项 不要使用filter 会导致重复删除
              this.leakTasks.splice(this.leakTasks.indexOf(vo),1)
            }})
          })
      }.layoutWeight(1)
      .width('100%')
      .scrollBar(BarState.Off)
      .divider({
        color:Color.Transparent,
        strokeWidth:16
      })
    }.width('100%')
    .height('100%')
    .alignItems(HorizontalAlign.Start)
  }
}
