import { Table ,Column} from "@zxhhyj/storm";
import { NodeRef } from "libleakguard.so";

/**
 * 分析任务信息接口
 */
// 数据库表结构设计

// 分析任务表
export interface AnalysisTask {
  /** 任务ID */
  taskId?: number;
  /** 堆快照文件路径 */
  heapSnapshotPath: string;
  /** 任务创建时间 */
  createTime: Date;
  /** 任务完成时间 */
  completeTime?: Date;
  /** 任务状态 (1: 进行中, 2: 完成, 3: 失败) */
  status: number;
  /** 是否查看 */
  isViewed: boolean;
  /** 分析结果 - 引用路径列表 */
  referencePaths?: NodeRef[];
}

export class AnalysisTaskTable extends Table<AnalysisTask>{
  override readonly tableName: string = 'analysis_task'
  readonly taskId = Column.integer('task_id').primaryKey(true).bindTo(this, 'taskId')
  readonly heapSnapshotPath = Column.text('heap_snapshot_path').notNull().bindTo(this, 'heapSnapshotPath')
  readonly status = Column.integer('status').notNull().bindTo(this, 'status')
  readonly isViewed = Column.boolean('is_viewed').notNull().bindTo(this, 'isViewed')
  readonly createTime = Column.timestamp('create_time').notNull().bindTo(this, 'createTime')
  readonly completeTime = Column.timestamp('complete_time').bindTo(this, 'completeTime')
  readonly referencePaths = Column.text('reference_paths',{
    restore:(it:string)=>JSON.parse(it),
    save:(it:NodeRef[])=>JSON.stringify(it)
  }).bindTo(this, 'referencePaths')
}

export const analysisTaskTable = new AnalysisTaskTable()
