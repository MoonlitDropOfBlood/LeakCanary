import { emitter } from "@kit.BasicServicesKit";

const LeakTest:object[] = []

@ComponentV2
export struct Custom1Component{

  aboutToAppear(): void {
    emitter.on("test", this.a)
    LeakTest.push(this)
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