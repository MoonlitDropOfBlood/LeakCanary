import collections from "@arkts.collections";

@Sendable
export class ResultData {
  nodeRefs: collections.Array<NodeRefShare>;
  constructor(nodeRefs: collections.Array<NodeRefShare>) {
    this.nodeRefs = nodeRefs;
  }
}

@Sendable
export class ReferenceChainNodeShare{
  nodeId: number;
  name: string;
  type: string;
  path: string;
  line: number;

  constructor(nodeId: number, name: string, type: string, path: string, line: number) {
    this.nodeId = nodeId;
    this.name = name;
    this.type = type;
    this.path = path;
    this.line = line;
  }

}

@Sendable
export class ReferenceChainShare {
  from: ReferenceChainNodeShare;
  edgeType: string;
  to: ReferenceChainNodeShare;
  constructor(from: ReferenceChainNodeShare, edgeType: string, to: ReferenceChainNodeShare) {
    this.from = from;
    this.edgeType = edgeType;
    this.to = to;
  }
}

@Sendable
export class NodeRefShare{
  /** 节点哈希值 */
  hash: number;
  /** 节点名称 */
  name: string;
  /** 到GC根的引用链 */
  ref: collections.Array<ReferenceChainShare>;

  constructor(hash: number, name: string, ref: collections.Array<ReferenceChainShare>) {
    this.hash = hash;
    this.name = name;
    this.ref = ref;
  }
}
