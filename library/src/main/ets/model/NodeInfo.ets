import { util } from "@kit.ArkTS"

export class  NodeInfo{
  hash:number
  name:string
  target:object

  constructor(target: object) {
    this.hash = util.getHash(target)
    this.target = target
    this.name = target.constructor.name
  }

}