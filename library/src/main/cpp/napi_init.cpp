#include <string>
#include <vector>
#include <cstring>
#include <thread>
#include <future>
#include <cstdio>
#include "napi/native_api.h"
// 声明而非包含heap_snapshot_parser.cpp
#include "heap_snapshot_parser.h"
#include "rawheap_translate.h"

// 实现NAPI接口

// 创建任务
static napi_value CreateTask(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};

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

    char *filePathBuffer = new char[filePathLength + 1];
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
    napi_value args[1] = {nullptr};

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
    napi_value args[3] = {nullptr, nullptr, nullptr};

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

    char *nodeNameBuffer = new char[nodeNameLength + 1];
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
    TaskHeapSnapshot *task = TaskManager::getTask(taskId);
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
        const ReferenceChain &refChain = resultChains[i];
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
        napi_create_string_utf8(env, refChain.current_node.name.c_str(), refChain.current_node.name.length(),
                                &currentNodeName);
        napi_set_named_property(env, currentNodeObj, "name", currentNodeName);

        // 设置type
        napi_value currentNodeType;
        napi_create_string_utf8(env, refChain.current_node.type.c_str(), refChain.current_node.type.length(),
                                &currentNodeType);
        napi_set_named_property(env, currentNodeObj, "type", currentNodeType);

        // 处理path字段，可能为空字符串
        napi_value currentNodePath;
        napi_create_string_utf8(env, refChain.current_node.path.c_str(), refChain.current_node.path.length(),
                                &currentNodePath);
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
    napi_value args[2] = {nullptr};

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

    char *inFilePathBuffer = new char[inFilePathLength + 1];
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

    char *outFilePathBuffer = new char[outFilePathLength + 1];
    if (napi_get_value_string_utf8(env, args[1], outFilePathBuffer, outFilePathLength + 1, nullptr) != napi_ok) {
        delete[] outFilePathBuffer;
        return nullptr;
    }

    std::string outFilePath(outFilePathBuffer);
    delete[] outFilePathBuffer;

    rawheap_translate::RawHeap::TranslateRawheap(inFilePath, outFilePath);

    return nullptr;
}

// 定义NodeRef结构体用于存储结果
struct NodeRef {
    int hash;
    std::string name;
    std::vector<ReferenceChain> refs;
};

// 定义异步任务的数据结构
struct RawAnalyzeHashAsyncData {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    std::string file;
    std::vector<std::pair<std::string, int>> nodeInfos;
    std::vector<NodeRef> result;
    std::string error;
};

static void heapAnalyzeHashExecute(napi_env env, void *data) {
    RawAnalyzeHashAsyncData *asyncData = static_cast<RawAnalyzeHashAsyncData *>(data);
    // 创建任务
    int taskId = TaskManager::createTask(asyncData->file);
    if (taskId == -1) {
        asyncData->error = "创建任务失败";
        return;
    }
    try {
        // 处理每个节点信息
        for (const auto &nodeInfo : asyncData->nodeInfos) {
            std::string nodeName = "Int:" + std::to_string(nodeInfo.second);
            std::vector<ReferenceChain> refChains =
                TaskManager::getTask(taskId)->getShortestPathToGCRootByName(nodeName, 10);

            if (!refChains.empty()) {
                NodeRef nodeRef;
                nodeRef.hash = nodeInfo.second;
                nodeRef.name = nodeInfo.first;
                nodeRef.refs = refChains;
                asyncData->result.push_back(nodeRef);
            }
        }

        // 销毁任务
        TaskManager::destroyTask(taskId);
    } catch (...) {
        TaskManager::destroyTask(taskId);
        throw;
    }
}

// 异步任务执行函数
static void RawAnalyzeHashExecute(napi_env env, void *data) {
    RawAnalyzeHashAsyncData *asyncData = static_cast<RawAnalyzeHashAsyncData *>(data);

    try {
        // 转换rawheap文件为heapsnapshot
        std::string heapsnapshotFile = asyncData->file;
        size_t pos = heapsnapshotFile.rfind(".rawheap");
        if (pos != std::string::npos) {
            heapsnapshotFile.replace(pos, 8, ".heapsnapshot");
        }

        rawheap_translate::RawHeap::TranslateRawheap(asyncData->file, heapsnapshotFile);
        asyncData->file = heapsnapshotFile;
        heapAnalyzeHashExecute(env, asyncData);
        remove(heapsnapshotFile.c_str());
    } catch (const std::exception &e) {
        asyncData->error = e.what();
    } catch (...) {
        asyncData->error = "未知错误";
    }
}

// 辅助函数：解析函数参数
static bool parseAnalyzeHashParams(napi_env env, napi_callback_info info, std::string &filePath, std::vector<std::pair<std::string, int>> &nodeInfos) {
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    // 获取参数
    if (napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) != napi_ok) {
        return false;
    }

    if (argc < 2) {
        napi_throw_error(env, nullptr, "需要两个参数: 文件路径和节点信息数组");
        return false;
    }

    // 解析文件路径参数
    size_t filePathLength = 0;
    if (napi_get_value_string_utf8(env, args[0], nullptr, 0, &filePathLength) != napi_ok) {
        return false;
    }

    char *filePathBuffer = new char[filePathLength + 1];
    if (napi_get_value_string_utf8(env, args[0], filePathBuffer, filePathLength + 1, nullptr) != napi_ok) {
        delete[] filePathBuffer;
        return false;
    }

    filePath = std::string(filePathBuffer);
    delete[] filePathBuffer;

    // 解析节点信息数组
    uint32_t arrayLength = 0;
    if (napi_get_array_length(env, args[1], &arrayLength) != napi_ok) {
        napi_throw_error(env, nullptr, "解析节点信息数组失败");
        return false;
    }

    for (uint32_t i = 0; i < arrayLength; i++) {
        napi_value nodeInfoObj;
        if (napi_get_element(env, args[1], i, &nodeInfoObj) != napi_ok) {
            napi_throw_error(env, nullptr, "获取节点信息失败");
            return false;
        }

        // 获取name
        napi_value nameValue;
        if (napi_get_named_property(env, nodeInfoObj, "name", &nameValue) != napi_ok) {
            napi_throw_error(env, nullptr, "获取节点名称失败");
            return false;
        }

        size_t nameLength = 0;
        if (napi_get_value_string_utf8(env, nameValue, nullptr, 0, &nameLength) != napi_ok) {
            napi_throw_error(env, nullptr, "解析节点名称失败");
            return false;
        }

        char *nameBuffer = new char[nameLength + 1];
        if (napi_get_value_string_utf8(env, nameValue, nameBuffer, nameLength + 1, nullptr) != napi_ok) {
            delete[] nameBuffer;
            napi_throw_error(env, nullptr, "解析节点名称失败");
            return false;
        }

        std::string name(nameBuffer);
        delete[] nameBuffer;

        // 获取hash
        napi_value hashValue;
        if (napi_get_named_property(env, nodeInfoObj, "hash", &hashValue) != napi_ok) {
            napi_throw_error(env, nullptr, "获取节点哈希值失败");
            return false;
        }

        int32_t hash = 0;
        if (napi_get_value_int32(env, hashValue, &hash) != napi_ok) {
            napi_throw_error(env, nullptr, "解析节点哈希值失败");
            return false;
        }

        nodeInfos.emplace_back(name, hash);
    }

    return true;
}

// 异步任务完成回调
static void RawAnalyzeHashComplete(napi_env env, napi_status status, void *data) {
    RawAnalyzeHashAsyncData *asyncData = static_cast<RawAnalyzeHashAsyncData *>(data);

    napi_value result;
    if (!asyncData->error.empty()) {
        // 拒绝Promise
        napi_value error;
        napi_create_string_utf8(env, asyncData->error.c_str(), asyncData->error.length(), &error);
        napi_reject_deferred(env, asyncData->deferred, error);
    } else {
        // 解析结果为NAPI对象
        napi_create_array_with_length(env, asyncData->result.size(), &result);

        for (size_t i = 0; i < asyncData->result.size(); i++) {
            const NodeRef &nodeRef = asyncData->result[i];

            // 创建NodeRef对象
            napi_value nodeRefObj;
            napi_create_object(env, &nodeRefObj);

            // 设置hash
            napi_value hash;
            napi_create_int32(env, nodeRef.hash, &hash);
            napi_set_named_property(env, nodeRefObj, "hash", hash);

            // 设置name
            napi_value name;
            napi_create_string_utf8(env, nodeRef.name.c_str(), nodeRef.name.length(), &name);
            napi_set_named_property(env, nodeRefObj, "name", name);

            // 设置ref数组
            napi_value refArray;
            napi_create_array_with_length(env, nodeRef.refs.size(), &refArray);

            for (size_t j = 0; j < nodeRef.refs.size(); j++) {
                const ReferenceChain &refChain = nodeRef.refs[j];

                // 创建ReferenceChain对象
                napi_value refChainObj;
                napi_create_object(env, &refChainObj);

                // 设置from
                napi_value fromObj;
                napi_create_object(env, &fromObj);

                napi_value fromNodeId;
                napi_create_int32(env, refChain.referrer.id, &fromNodeId);
                napi_set_named_property(env, fromObj, "nodeId", fromNodeId);

                napi_value fromName;
                napi_create_string_utf8(env, refChain.referrer.name.c_str(), refChain.referrer.name.length(),
                                        &fromName);
                napi_set_named_property(env, fromObj, "name", fromName);

                napi_value fromType;
                napi_create_string_utf8(env, refChain.referrer.type.c_str(), refChain.referrer.type.length(),
                                        &fromType);
                napi_set_named_property(env, fromObj, "type", fromType);

                napi_value fromPath;
                napi_create_string_utf8(env, refChain.referrer.path.c_str(), refChain.referrer.path.length(),
                                        &fromPath);
                napi_set_named_property(env, fromObj, "path", fromPath);

                napi_value fromLine;
                napi_create_int32(env, refChain.referrer.line, &fromLine);
                napi_set_named_property(env, fromObj, "line", fromLine);

                napi_set_named_property(env, refChainObj, "from", fromObj);

                // 设置edgeType
                napi_value edgeType;
                napi_create_string_utf8(env, refChain.edge_type.c_str(), refChain.edge_type.length(), &edgeType);
                napi_set_named_property(env, refChainObj, "edgeType", edgeType);

                // 设置to
                napi_value toObj;
                napi_create_object(env, &toObj);

                napi_value toNodeId;
                napi_create_int32(env, refChain.current_node.id, &toNodeId);
                napi_set_named_property(env, toObj, "nodeId", toNodeId);

                napi_value toName;
                napi_create_string_utf8(env, refChain.current_node.name.c_str(), refChain.current_node.name.length(),
                                        &toName);
                napi_set_named_property(env, toObj, "name", toName);

                napi_value toType;
                napi_create_string_utf8(env, refChain.current_node.type.c_str(), refChain.current_node.type.length(),
                                        &toType);
                napi_set_named_property(env, toObj, "type", toType);

                napi_value toPath;
                napi_create_string_utf8(env, refChain.current_node.path.c_str(), refChain.current_node.path.length(),
                                        &toPath);
                napi_set_named_property(env, toObj, "path", toPath);

                napi_value toLine;
                napi_create_int32(env, refChain.current_node.line, &toLine);
                napi_set_named_property(env, toObj, "line", toLine);

                napi_set_named_property(env, refChainObj, "to", toObj);

                // 添加到ref数组
                napi_set_element(env, refArray, j, refChainObj);
            }

            napi_set_named_property(env, nodeRefObj, "ref", refArray);

            // 添加到结果数组
            napi_set_element(env, result, i, nodeRefObj);
        }

        // 解决Promise
        napi_resolve_deferred(env, asyncData->deferred, result);
    }

    // 清理资源
    napi_delete_async_work(env, asyncData->work);
    delete asyncData;
}


// 辅助函数：创建和启动异步工作
static napi_value createAndStartAsyncWork(napi_env env, const std::string &filePath, 
                                          std::vector<std::pair<std::string, int>> &&nodeInfos,
                                          napi_async_execute_callback executeFunc,
                                          const char *resourceNameStr) {
    // 创建异步数据
    RawAnalyzeHashAsyncData *asyncData = new RawAnalyzeHashAsyncData();
    asyncData->env = env;
    asyncData->file = filePath;
    asyncData->nodeInfos = std::move(nodeInfos);

    // 创建Promise
    napi_value promise;
    if (napi_create_promise(env, &asyncData->deferred, &promise) != napi_ok) {
        delete asyncData;
        return nullptr;
    }

    // 创建异步工作
    napi_value resourceName;
    napi_create_string_utf8(env, resourceNameStr, NAPI_AUTO_LENGTH, &resourceName);

    if (napi_create_async_work(env, nullptr, resourceName, executeFunc, RawAnalyzeHashComplete, asyncData,
                               &asyncData->work) != napi_ok) {
        napi_reject_deferred(env, asyncData->deferred, nullptr);
        delete asyncData;
        return nullptr;
    }

    // 排队执行异步工作
    if (napi_queue_async_work(env, asyncData->work) != napi_ok) {
        napi_delete_async_work(env, asyncData->work);
        napi_reject_deferred(env, asyncData->deferred, nullptr);
        delete asyncData;
        return nullptr;
    }

    return promise;
}

// 异步rawAnalyzeHash函数
static napi_value RawAnalyzeHash(napi_env env, napi_callback_info info) {
    std::string filePath;
    std::vector<std::pair<std::string, int>> nodeInfos;

    // 解析参数
    if (!parseAnalyzeHashParams(env, info, filePath, nodeInfos)) {
        return nullptr;
    }

    // 创建并启动异步工作
    return createAndStartAsyncWork(env, filePath, std::move(nodeInfos), RawAnalyzeHashExecute, "RawAnalyzeHashAsync");
}

static napi_value HeapAnalyzeHash(napi_env env, napi_callback_info info) {
    std::string filePath;
    std::vector<std::pair<std::string, int>> nodeInfos;

    // 解析参数
    if (!parseAnalyzeHashParams(env, info, filePath, nodeInfos)) {
        return nullptr;
    }

    // 创建并启动异步工作
    return createAndStartAsyncWork(env, filePath, std::move(nodeInfos), heapAnalyzeHashExecute, "HeapAnalyzeHashAsync");
}


EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    // 定义导出的方法
    napi_property_descriptor desc[] = {
        {"createTask", nullptr, CreateTask, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"destroyTask", nullptr, DestroyTask, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getShortestPathToGCRoot", nullptr, GetShortestPathToGCRoot, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"rawHeapTranslate", nullptr, rawHeapTranslate, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"rawAnalyzeHash", nullptr, RawAnalyzeHash, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"heapAnalyzeHash", nullptr, HeapAnalyzeHash, nullptr, nullptr, nullptr, napi_default, nullptr}};
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

// 模块注册信息
static napi_module leakModule = {.nm_version = 1,
                                 .nm_flags = 0,
                                 .nm_filename = nullptr,
                                 .nm_register_func = Init,
                                 .nm_modname = "leakguard",
                                 .nm_priv = nullptr,
                                 .reserved = {0}};

// 模块注册入口
extern "C" __attribute__((constructor)) void RegisterModule(void) { napi_module_register(&leakModule); }