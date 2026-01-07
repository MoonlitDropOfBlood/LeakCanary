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
  nodeId: number;
  name: string;
  type: string;
  path: string;
  line: number;
}

// 引用链接口
export interface ReferenceChain {
  from: ReferenceChainNode;
  edgeType: string;
  to: ReferenceChainNode;
}
// 堆快照解析结果类型
export interface  HeapSnapshotResult  {
  [className: string]: ReferenceChainNode[][];
}

// 创建内存快照分析任务
export const createTask: (filePath: string) => number;

// 销毁内存快照分析任务
export const destroyTask: (taskId: number) => boolean;

// 获取到GC根的最短引用链
export const getShortestPathToGCRoot: (taskId: number, nodeName: string, maxPaths?: number) => HeapSnapshotResult[][][];
