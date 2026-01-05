import { LeakCanary } from "@duke/leak-canary";
import { emitter } from "@kit.BasicServicesKit";
import { a } from "../A";

@ComponentV2
export struct Custom1Component{

  aboutToAppear(): void {
    emitter.on("test", this.a)
    a.push(this)
  }

  a(){
    console.log("test")
  }

  onDidBuild(): void {
    console.log('did build')
  }


  build() {
    Column(){}
    .width(10)
    .height(10)
  }
}