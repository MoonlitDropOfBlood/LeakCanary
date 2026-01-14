#include <string>
#include <vector>
#include <cstring>
#include "napi/native_api.h"
// 声明而非包含heap_snapshot_parser.cpp
#include "heap_snapshot_parser.h"
#include "rawheap_translate.h"

// 实现NAPI接口

// 创建任务
static napi_value CreateTask(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    
    // 获取参数
    if (napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) != napi_ok) {
        return nullptr;
    }
    
    if (argc < 1) {
        napi_throw_error(env, nullptr, "需要一个参数: 文件路径");
        return nullptr;
    }
    
    // 解析文件路径参数
    size_t filePathLength = 0;
    if (napi_get_value_string_utf8(env, args[0], nullptr, 0, &filePathLength) != napi_ok) {
        return nullptr;
    }
    
    char* filePathBuffer = new char[filePathLength + 1];
    if (napi_get_value_string_utf8(env, args[0], filePathBuffer, filePathLength + 1, nullptr) != napi_ok) {
        delete[] filePathBuffer;
        return nullptr;
    }
    
    std::string filePath(filePathBuffer);
    delete[] filePathBuffer;
    
    // 创建任务
    int taskId = TaskManager::createTask(filePath);
    
    // 返回任务ID
    napi_value result;
    napi_create_int32(env, taskId, &result);
    return result;
}

// 销毁任务
static napi_value DestroyTask(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    
    // 获取参数
    if (napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) != napi_ok) {
        return nullptr;
    }
    
    if (argc < 1) {
        napi_throw_error(env, nullptr, "需要一个参数: 任务ID");
        return nullptr;
    }
    
    // 解析任务ID参数
    int32_t taskId = 0;
    if (napi_get_value_int32(env, args[0], &taskId) != napi_ok) {
        return nullptr;
    }
    
    // 销毁任务
    bool success = TaskManager::destroyTask(taskId);
    
    // 返回结果
    napi_value result;
    napi_get_boolean(env, success, &result);
    return result;
}

// 获取最短引用链到GC根
static napi_value GetShortestPathToGCRoot(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3] = { nullptr, nullptr, nullptr };
    
    // 获取参数
    if (napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) != napi_ok) {
        return nullptr;
    }
    
    if (argc < 2) {
        napi_throw_error(env, nullptr, "需要两个参数: 任务ID和节点ID");
        return nullptr;
    }
    
    // 解析任务ID参数
    int32_t taskId = 0;
    if (napi_get_value_int32(env, args[0], &taskId) != napi_ok) {
        return nullptr;
    }
    
    // 解析节点name参数
    size_t nodeNameLength = 0;
    if (napi_get_value_string_utf8(env, args[1], nullptr, 0, &nodeNameLength) != napi_ok) {
        return nullptr;
    }
    
    char* nodeNameBuffer = new char[nodeNameLength + 1];
    if (napi_get_value_string_utf8(env, args[1], nodeNameBuffer, nodeNameLength + 1, nullptr) != napi_ok) {
        delete[] nodeNameBuffer;
        return nullptr;
    }
    
    std::string nodeName(nodeNameBuffer);
    delete[] nodeNameBuffer;
    
    // 解析最大路径数参数
    int32_t maxDepth = 5; // 默认最大路径数
    if (argc >= 3) {
        if (napi_get_value_int32(env, args[2], &maxDepth) != napi_ok) {
            return nullptr;
        }
    }
    
    // 获取任务
    TaskHeapSnapshot* task = TaskManager::getTask(taskId);
    if (!task) {
        napi_throw_error(env, nullptr, "无效的任务ID");
        return nullptr;
    }
    
    // 查找最短引用链
     std::vector<ReferenceChain> resultChains = task->getShortestPathToGCRootByName(nodeName, maxDepth);
    
    // 将结果转换为NAPI数组
    napi_value result;
    napi_create_array_with_length(env, resultChains.size(), &result);
    
    for (size_t i = 0; i < resultChains.size(); i++) {
        const ReferenceChain& refChain = resultChains[i];
        // 创建链对象
        napi_value chainObj;
        napi_create_object(env, &chainObj);
        
        // 设置引用者信息
        napi_value referrerObj;
        napi_create_object(env, &referrerObj);
        
        napi_value referrerNodeId;
        napi_create_int32(env, refChain.referrer.id, &referrerNodeId);
        napi_set_named_property(env, referrerObj, "nodeId", referrerNodeId);
        
        napi_value referrerName;
        napi_create_string_utf8(env, refChain.referrer.name.c_str(), refChain.referrer.name.length(), &referrerName);
        napi_set_named_property(env, referrerObj, "name", referrerName);
        
        napi_value referrerType;
        napi_create_string_utf8(env, refChain.referrer.type.c_str(), refChain.referrer.type.length(), &referrerType);
        napi_set_named_property(env, referrerObj, "type", referrerType);
        
        // 处理path字段，可能为空字符串
        napi_value referrerPath;
        napi_create_string_utf8(env, refChain.referrer.path.c_str(), refChain.referrer.path.length(), &referrerPath);
        napi_set_named_property(env, referrerObj, "path", referrerPath);
        
        // 处理line字段，可能为0
        napi_value referrerLine;
        napi_create_int32(env, refChain.referrer.line, &referrerLine);
        napi_set_named_property(env, referrerObj, "line", referrerLine);
        
        // 设置引用者
        napi_set_named_property(env, chainObj, "from", referrerObj);
        
        // 设置edgeType
        napi_value edgeTypeVal;
        napi_create_string_utf8(env, refChain.edge_type.c_str(), refChain.edge_type.length(), &edgeTypeVal);
        napi_set_named_property(env, chainObj, "edgeType", edgeTypeVal);
            
        // 设置被引用者信息
        napi_value currentNodeObj;
        napi_create_object(env, &currentNodeObj);
        
        // 设置nodeId
        napi_value currentNodeId;
        napi_create_int32(env, refChain.current_node.id, &currentNodeId);
        napi_set_named_property(env, currentNodeObj, "nodeId", currentNodeId);
                
        // 设置name
        napi_value currentNodeName;
        napi_create_string_utf8(env, refChain.current_node.name.c_str(), refChain.current_node.name.length(), &currentNodeName);
        napi_set_named_property(env, currentNodeObj, "name", currentNodeName);
        
        // 设置type
        napi_value currentNodeType;
        napi_create_string_utf8(env, refChain.current_node.type.c_str(), refChain.current_node.type.length(), &currentNodeType);
        napi_set_named_property(env, currentNodeObj, "type", currentNodeType);
                
        // 处理path字段，可能为空字符串
        napi_value currentNodePath;
        napi_create_string_utf8(env, refChain.current_node.path.c_str(), refChain.current_node.path.length(), &currentNodePath);
        napi_set_named_property(env, currentNodeObj, "path", currentNodePath);
                
        // 处理line字段，可能为0
        napi_value currentNodeLine;
        napi_create_int32(env, refChain.current_node.line, &currentNodeLine);
        napi_set_named_property(env, currentNodeObj, "line", currentNodeLine);
            
        // 设置被引用者
        napi_set_named_property(env, chainObj, "to", currentNodeObj);
                
        // 将链对象添加到链数组
        napi_set_element(env, result, i, chainObj);
        
    }
    
    return result;
}

static napi_value rawHeapTranslate(napi_env env, napi_callback_info info) {
       size_t argc = 2;
    napi_value args[2] = { nullptr };
    
    // 获取参数
    if (napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) != napi_ok) {
        return nullptr;
    }
    
    if (argc < 2) {
        napi_throw_error(env, nullptr, "需要一个参数: 文件路径");
        return nullptr;
    }
    
    // 解析入口文件路径参数
    size_t inFilePathLength = 0;
    if (napi_get_value_string_utf8(env, args[0], nullptr, 0, &inFilePathLength) != napi_ok) {
        return nullptr;
    }
    
    char* inFilePathBuffer = new char[inFilePathLength + 1];
    if (napi_get_value_string_utf8(env, args[0], inFilePathBuffer, inFilePathLength + 1, nullptr) != napi_ok) {
        delete[] inFilePathBuffer;
        return nullptr;
    }
    
    std::string inFilePath(inFilePathBuffer);
    delete[] inFilePathBuffer;

    // 解析出口文件路径参数
    size_t outFilePathLength = 0;
    if (napi_get_value_string_utf8(env, args[1], nullptr, 0, &outFilePathLength) != napi_ok) {
        return nullptr;
    }
    
    char* outFilePathBuffer = new char[outFilePathLength + 1];
    if (napi_get_value_string_utf8(env, args[1], outFilePathBuffer, outFilePathLength + 1, nullptr) != napi_ok) {
        delete[] outFilePathBuffer;
        return nullptr;
    }
    
    std::string outFilePath(outFilePathBuffer);
    delete[] outFilePathBuffer;

    rawheap_translate::RawHeap::TranslateRawheap(inFilePath,outFilePath);

    return nullptr;
}



EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    // 定义导出的方法
    napi_property_descriptor desc[] = {
        {"createTask", nullptr, CreateTask, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"destroyTask", nullptr, DestroyTask, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getShortestPathToGCRoot", nullptr, GetShortestPathToGCRoot, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"rawHeapTranslate", nullptr, rawHeapTranslate, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

// 模块注册信息
static napi_module leakModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "leakguard",
    .nm_priv = nullptr,
    .reserved = { 0 }
};

// 模块注册入口
extern "C" __attribute__((constructor)) void RegisterModule(void) {
    napi_module_register(&leakModule);
}