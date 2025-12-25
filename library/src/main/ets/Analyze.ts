import { parseHeapSnapshotAndFindChains, parseHeapSnapshotAndFindChainsForObjects } from "libleakcanary.so"

export function analyze(file: string, objects: object[]) {
  let chains = parseHeapSnapshotAndFindChainsForObjects(file, objects)
  console.log(JSON.stringify(chains))
}