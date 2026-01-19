export interface ObjInfo{
  hash:number
  name:string
  msg?:string
}

export interface LeakInfo{
  leakObjList:ObjInfo[]
  snapshot_hash:string,
  version:string
}
