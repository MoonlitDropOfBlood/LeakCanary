import { AnalysisTask } from "../db/DatabaseInterfaces"

@ObservedV2
export class TaskVO {
  readonly taskId: string
  @Trace
  isViewed: boolean
  readonly task:AnalysisTask
  constructor(task:AnalysisTask) {
    this.task = task
    this.taskId = task.taskId?.toString() ?? ''
    this.isViewed = task.isViewed
  }
}