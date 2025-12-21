import { parseHeapSnapshotAndFindChains } from "libleakcanary.so"
import { taskpool } from "@kit.ArkTS"

export function analyze(file: string, classNames: string[]) {
  let task = new taskpool.Task(analyzeHeapSnapshot, file, classNames)
  taskpool.execute(task, taskpool.Priority.HIGH)
}

@Concurrent
function analyzeHeapSnapshot(file: string, classNames: string[]) {
  let set = new Set<string>()
  classNames.forEach(name => set.add(name))
  classNames = Array.from(set)
  let chains = parseHeapSnapshotAndFindChains(file, classNames)
  console.log(JSON.stringify(chains))
}