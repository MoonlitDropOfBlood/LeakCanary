// 引用边的类型枚举
export enum EdgeType {
  CONTEXT = "context",
  ELEMENT = "element",
  PROPERTY = "property",
  INTERNAL = "internal",
  HIDDEN = "hidden",
  SHORTCUT = "shortcut",
  WEAK = "weak"
}

// 引用链节点接口
export interface ReferenceChainNode {
  className: string;
  edgeType: EdgeType;
  propertyName: string;
}

// 堆快照解析结果类型
export type HeapSnapshotResult = {
  [className: string]: ReferenceChainNode[][];
}
// 解析堆快照并查找指定类的引用链
export const parseHeapSnapshotAndFindChains: (
  filePath: string,
  classNames: string[]
) => HeapSnapshotResult;