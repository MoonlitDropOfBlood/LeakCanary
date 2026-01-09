import { AnalysisTask } from "../db/DatabaseInterfaces";

@Entry({routeName: 'LeakDetailsPage'})
@ComponentV2
export struct LeakDetails{

  private task:AnalysisTask = this.getUIContext().getRouter().getParams() as AnalysisTask

  build() {
    Column(){
      Text(this.task.heapSnapshotPath)
        .fontSize(24)
    }.width('100%')
    .height('100%')
  }
}