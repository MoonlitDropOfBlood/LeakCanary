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

export interface HashInfo{
  hash:number
  name:string
}

// 引用链节点接口
export interface ReferenceChainNode {
  nodeId: number;
  name: string;
  type: string;
  path: string;
  line: number;
}

/**
 * 节点引用信息接口
 */
export interface NodeRef {
  /** 节点哈希值 */
  hash: number;
  /** 节点名称 */
  name: string;
  /** 到GC根的引用链 */
  ref: ReferenceChain[];
}

// 引用链接口
export interface ReferenceChain {
  from: ReferenceChainNode;
  edgeType: string;
  to: ReferenceChainNode;
}

// 创建内存快照分析任务
export const createTask: (filePath: string) => number;

// 销毁内存快照分析任务
export const destroyTask: (taskId: number) => boolean;

// 获取到GC根的最短引用链
export const getShortestPathToGCRoot: (taskId: number, name: string, maxDepth?: number) => ReferenceChain[];

// 二进制转成快照文件
export const rawHeapTranslate: (filePath: string, outFilePath:string) => void;

// 分析raw内存快照中指定对象的引用链
export const rawAnalyzeHash: (filePath: string, hashInfos:HashInfo[]) => Promise<NodeRef[]>;

// 分析内存快照中指定对象的引用链
export const heapAnalyzeHash: (filePath: string, hashInfos:HashInfo[]) => Promise<NodeRef[]>;
