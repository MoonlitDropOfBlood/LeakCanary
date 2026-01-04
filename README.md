# LeakCanary Library for OpenHarmony

## 简介

[![openHarmony](https://img.shields.io/badge/openharmony-v1.3.0-brightgreen)](https://gitee.com/Duke_Bit/leak-canary/releases/tag/v1.3.0)

LeakCanary是一个为OpenHarmony开发的内存泄漏检测库，提供自动化的内存泄漏监控和检测功能。

- 支持自动检测组件内存泄漏
- 基于`WeakMap`和`FinalizationRegistry`实现高效的内存监控
- 基于Navigation的页面，实时监控页面及其子组件的内存状态
- 内存泄漏发送时会在Log中打印一条Tag为GC的error level log

## 下载安装

````
ohpm install @duke/leak-canary
````

OpenHarmony ohpm
环境配置等更多内容，请参考[如何安装 OpenHarmony ohpm 包](https://gitee.com/openharmony-tpc/docs/blob/master/OpenHarmony_har_usage.md)

## 使用说明

### 基本用法

导入LeakCanary库：

```typescript
import { LeakCanary } from '@duke/leak-canary';
```

### 核心API

#### 全局初始化监控（推荐）API 20开始

在 EntryAbility 中初始化 LeakCanary 全局监控：

```typescript
LeakCanary.initRegisterGlobalWatch();
```

#### 初始化监控 API 20(不含)以下

初始化LeakCanary并传入根组件：

```typescript
LeakCanary.registerRootWatch(rootComponent);
```


#### 手动注册组件（可选）

手动注册需要监控的组件（仅在特殊场景下使用）：

```typescript
LeakCanary.registerComponent(component);
```

### LeakCanary

| 方法名                     | 入参                    | 接口描述                         |
|:------------------------|:----------------------|:-----------------------------|
| initRegisterGlobalWatch | -                     | 全局初始化内存泄漏监控，自动监听所有自定义组件      |
| registerRootWatch       | rootComponent: object | 注册Navigation根组件进行内存泄漏监控（已弃用） |
| registerComponent       | component: object     | 手动注册监听，不用考虑时机（已弃用）           |
### 工作原理

LeakCanary通过以下方式实现内存泄漏检测：

1. 利用 `WeakMap` 和 `WeakRef` 弱引用机制避免干扰正常GC
2. 通过 `FinalizationRegistry` 监听对象被GC的时机
3. 集成OpenHarmony的UI观察者系统，监听导航状态变化
4. 在页面即将消失时自动注册该页面的所有子组件进行监控



## 约束与限制

- 受限于内部API的限制，目前仅支持API 20(含)HarmonyOS6.0以上的设备 进行全局内存泄漏监控
- 对于@Entry router 下的组件没有很好的自动监控方式，需要手动监控
- 基于目前仅在页面销毁前进行组件注册的方式，对于动态组件可能会监控不到，如if 不满足条件后，但是实际泄漏，则无法被检测到，如有需要，建议手动监控
- 该库不建议在生产环境使用，可能会增加卡顿或者ANR

在下述版本验证通过：

DevEco Studio: 6.0.0, SDK: HarmonyOS 6.0.0.120 Release Ohos_sdk_public 6.0.0.47 (API Version 20 Release)

## 规划

已完成：
* [已完成] 手动注册时不在需要考虑时机
* [已完成] 添加通知功能，检测到内存泄漏时，会弹出通知提示

受限：API 20以上支持全局自动监听所有自定义组件
* [已完成] ❗全局自动监听所有自定义组件
* [已完成] ❗对于List下复用的组件兼容性测试及支持
* [已完成] ❗动态组件监听的实现方案
* [已完成] ❗对于Page的自动监听支持

未完成：
* [未完成] 添加数据库，存储泄漏信息，提供查询功能
* [未完成] 添加内存泄漏检测报告页面，分近期和全部

## 目录结构

````
|---- WebSocket
|     |---- AppScrope  # 示例代码文件夹
|     |---- entry  # 示例代码文件夹
|---- library 
|     |---- example # 示例代码文件夹  
|     |---- src 
|            |---- main 
|                    |---- ets 
|                           |---- LeakCanary.ts # 主入口文件 
|                           |---- ObjWatch.ts # 内存监控核心实现 
|     |---- index.ets # 对外接口 
|     |---- README.md # 安装使用方法  
|     |---- CHANGELOG.md  # 更新日志
````

## 贡献代码

使用过程中发现任何问题都可以提 [Issue](https://gitee.com/Duke_Bit/leak-canary/issues)
给我，当然，我也非常欢迎你给我发 [PR](https://gitee.com/Duke_Bit/leak-canary) 。

## 开源协议

本项目基于 [MIT license](https://gitee.com/Duke_Bit/leak-canary/blob/master/LICENSE) ，请自由地享受和参与开源。

## 其他库

- [eventpost](https://ohpm.openharmony.cn/#/cn/detail/eventpost) 事件分发，支持组件中的lifecycle，在组件中使用自动取消订阅: [https://gitee.com/Duke_Bit/eventpost](https://gitee.com/Duke_Bit/eventpost)
- [@duke/view-model](https://ohpm.openharmony.cn/#/cn/detail/@duke%2Fview-model)
  鸿蒙版本的Lifecycle库，支持组件、Router和Navigation: [https://gitee.com/Duke_Bit/view-model](https://gitee.com/Duke_Bit/view-model)
- [@duke/logan-ext](https://ohpm.openharmony.cn/#/cn/detail/@duke%2Flogan-ext)
  Logan扩展库，方便开箱即用: [https://gitee.com/Duke_Bit/logan](https://gitee.com/Duke_Bit/logan)
- [@duke/logan](https://ohpm.openharmony.cn/#/cn/detail/@duke%2Flogan)
  Logan是一个为OpenHarmony开发的日志库，对美团技术团队的Logan的鸿蒙化移植版本: [https://gitee.com/Duke_Bit/logan](https://gitee.com/Duke_Bit/logan)
- [@duke/websocket-client](https://ohpm.openharmony.cn/#/cn/detail/@duke%2Fwebsocket-client)
  WebSocket库解决官方API的一些bug问题: [https://gitee.com/Duke_Bit/websocket](https://gitee.com/Duke_Bit/websocket)
- [@duke/component-lifecycle](https://ohpm.openharmony.cn/#/cn/detail/@duke%2Fcomponent-lifecycle)
  鸿蒙版本的Lifecycle库，支持组件、Router和Navigation: [https://gitee.com/Duke_Bit/component-lifecycle](https://gitee.com/Duke_Bit/component-lifecycle)
- [@duke/elf-dialog](https://ohpm.openharmony.cn/#/cn/detail/@duke%2Felf-dialog) CustomDialog黑魔法 不依赖promptAction
  实现的函数级弹窗，省去复杂的模版代码，让你专注于你的业务，一行代码搞定弹窗: [https://gitee.com/Duke_Bit/elf-dialog](https://gitee.com/Duke_Bit/elf-dialog)
- [@duke/elf-refresh](https://ohpm.openharmony.cn/#/cn/detail/@duke%2Felf-refresh) OpenHarmony
  刷新组件，支持下拉刷新和上拉加载更多，支持各种组件，List、Grid，支持header，footer，目标打造HarmonyOS的SmartRefreshLayout: [https://gitee.com/Duke_Bit/ElfRefresh](https://gitee.com/Duke_Bit/ElfRefresh)