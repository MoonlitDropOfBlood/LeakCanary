import { AnalysisTask } from "../db/DatabaseInterfaces"
import { ObjInfo } from "./ObjInfo"

export interface CheckTask{
  task:AnalysisTask
  objInfos:ObjInfo[]
}
