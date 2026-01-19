import { collections } from '@kit.ArkTS';

@Sendable
export class TransferData{
  nodeInfos:collections.Array<NodeInfo>
  file:string

  constructor(nodeInfos: collections.Array<NodeInfo>,file:string) {
    this.nodeInfos = nodeInfos;
    this.file = file
  }

}

@Sendable
export class NodeInfo{
  name:string
  hash:number

  constructor(name: string, hash: number) {
    this.name = name;
    this.hash = hash;
  }


}