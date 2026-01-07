import { createTask,destroyTask,getShortestPathToGCRoot, HeapSnapshotResult, } from "libleakcanary.so"
import { taskpool, util } from "@kit.ArkTS"
import hilog from "@ohos.hilog"

interface NodeInfo{
  hash:number
  name:string
}

interface NodeRef{
  hash:number
  name:string
  ref:HeapSnapshotResult[][]
}

export function analyze(file: string, objects: object[]) {
  const task = new taskpool.Task(analyzeHash,file,objects.map<NodeInfo>(it=>({
    hash:util.getHash(it),
    name:it.constructor.name
  })))
  taskpool.execute(task,taskpool.Priority.HIGH).catch(() => {
    hilog.error(0x0002, "Analyze", "analyzeHash error")
  }).then(()=>{
    hilog.debug(0x0002, "Analyze","analyzeHash done")
  })
}

@Concurrent
function analyzeHash(file: string, objects: NodeInfo[]) {
  // 按name字段对objects进行分组
  const groupedObjects = new Map<string, NodeInfo[]>()
  for (let obj of objects) {
    if (!groupedObjects.has(obj.name)) {
      groupedObjects.set(obj.name, [])
    }
    groupedObjects.get(obj.name)!.push(obj)
  }
  
  let task = createTask(file)
  groupedObjects.forEach((value:NodeInfo[],key:string)=>{
    const objsRef = getShortestPathToGCRoot(task, key)
    objsRef.map<NodeRef>((it,index:number)=>{
      return {
      hash:value[index]?.hash || 0,
      name:key,
      ref:it
    }}).forEach(it=>{
      hilog.debug(0x0002, "Analyze", "ref: %s", JSON.stringify(it))
    })
  })
  
  destroyTask(task)
}
