import { notificationManager } from "@kit.NotificationKit"
import { AbilityLifecycleCallback, common, UIAbility, wantAgent } from "@kit.AbilityKit"
import { router, window } from "@kit.ArkUI"

export class LeakNotification {
  //单例
  private static instance: LeakNotification = null
  public static getInstance(): LeakNotification {
    if (this.instance == null) {
      this.instance = new LeakNotification()
    }
    return this.instance
  }

  private wantAgentInfo:wantAgent.WantAgentInfo = null
  private lifecycleId:number = -1

  initPublisher(context:common.UIAbilityContext) {
    if (!notificationManager.isNotificationEnabledSync()) {//通知权限未开启
      return
    }
    if(this.wantAgentInfo){
      return
    }
    this.wantAgentInfo = this.createWantAgent(context.abilityInfo.bundleName,context.abilityInfo.name)
    notificationManager.publish({
      content:{
        notificationContentType: notificationManager.ContentType.NOTIFICATION_CONTENT_BASIC_TEXT,
        normal:{
          title:"LeakCanary",
          text:"正在检测内存泄漏"
        }
      },
      label:"LeakCanary",
      notificationSlotType: notificationManager.SlotType.SERVICE_INFORMATION,
      isAlertOnce:true,
      tapDismissed:false,
      wantAgent:this.wantAgentInfo,
    })
    let applicationContext = context.getApplicationContext();
    this.lifecycleId = applicationContext.on('abilityLifecycle', this.abilityLifecycleCallback);
  }

  private readonly abilityLifecycleCallback: AbilityLifecycleCallback = {
    onAbilityCreate(ability: UIAbility) {
    },
    onWindowStageCreate(ability: UIAbility, windowStage: window.WindowStage) {
      if(ability.launchWant.uri == 'leakcanary://leaknotification'){
        setTimeout(()=>{
          router.pushNamedRoute({name:'LeakTasksPage'})
        },1000)
        return
      }
    },
    onWindowStageActive(ability: UIAbility, windowStage: window.WindowStage) {
    },
    onWindowStageInactive(ability: UIAbility, windowStage: window.WindowStage) {
    },
    onWindowStageDestroy(ability: UIAbility, windowStage: window.WindowStage) {
    },
    onAbilityDestroy(ability: UIAbility) {
      ability.context.getApplicationContext().off('abilityLifecycle',LeakNotification.getInstance().lifecycleId)
    },
    onAbilityForeground(ability: UIAbility) {

    },
    onAbilityBackground(ability: UIAbility) {
    },
    onAbilityContinue(ability: UIAbility) {
    },
    onNewWant(ability: UIAbility) {
      if(ability.lastRequestWant.uri == 'leakcanary://leaknotification'){
        router.pushNamedRoute({name:'LeakTasksPage'})
        return
      }
    },
    onWillNewWant(ability: UIAbility) {
    },
    onAbilityWillCreate(ability: UIAbility) {
    },
    onWindowStageWillCreate(ability: UIAbility, windowStage: window.WindowStage) {
    },
    onWindowStageWillDestroy(ability: UIAbility, windowStage: window.WindowStage) {
    },
    onAbilityWillDestroy(ability: UIAbility) {
    },
    onAbilityWillForeground(ability: UIAbility) {
    },
    onAbilityWillBackground(ability: UIAbility) {
    },
    onAbilityWillContinue(ability: UIAbility) {
    },
    onWindowStageWillRestore(ability: UIAbility, windowStage: window.WindowStage) {
    },
    onWindowStageRestore(ability: UIAbility, windowStage: window.WindowStage) {
    },
    onAbilityWillSaveState(ability: UIAbility) {
    },
    onAbilitySaveState(ability: UIAbility) {
    }
  };


  private createWantAgent(bundleName:string,abilityName:string):wantAgent.WantAgentInfo {
    const  wantAgentInfo: wantAgent.WantAgentInfo = {
      // 点击通知后，将要执行的动作列表
      // 添加需要被拉起应用的bundleName和abilityName
      wants: [
        {
          bundleName: bundleName,
          abilityName: abilityName,
          uri: 'leakcanary://leaknotification',
        }
      ],
      // 指定点击通知栏消息后的动作是拉起ability
      actionType: wantAgent.OperationType.START_ABILITY,
      // 使用者自定义的一个私有值
      requestCode: 0,
      // 点击通知后，动作执行属性
      actionFlags: [wantAgent.WantAgentFlags.UPDATE_PRESENT_FLAG],
    }
    return wantAgentInfo
  }

  publishNotification(text: string) {
    notificationManager.publish({
      content:{
        notificationContentType: notificationManager.ContentType.NOTIFICATION_CONTENT_BASIC_TEXT,
        normal:{
          title:"LeakCanary",
          text:text
        }
      },
      label:"LeakCanary",
      notificationSlotType: notificationManager.SlotType.SERVICE_INFORMATION,
      isAlertOnce:true,
      tapDismissed:false,
      wantAgent:this.wantAgentInfo
    })
  }
}
