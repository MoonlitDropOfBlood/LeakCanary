#include <string>
#include <vector>
#include <map>
#include "napi/native_api.h"
// 声明而非包含heap_snapshot_parser.cpp
#include "heap_snapshot_parser.h"

// 边类型映射函数声明
std::string mapEdgeType(int typeCode);

namespace {
    // 将ReferenceChainNode转换为JavaScript对象
    napi_value CreateReferenceChainNode(napi_env env, const ReferenceChainNode& node) {
        napi_value jsNode;
        napi_create_object(env, &jsNode);
        
        // className
        napi_value classNameValue;
        napi_create_string_utf8(env, node.className.c_str(), NAPI_AUTO_LENGTH, &classNameValue);
        napi_set_named_property(env, jsNode, "className", classNameValue);
        
        // edgeType
        napi_value edgeTypeValue;
        napi_create_string_utf8(env, node.edgeType.c_str(), NAPI_AUTO_LENGTH, &edgeTypeValue);
        napi_set_named_property(env, jsNode, "edgeType", edgeTypeValue);
        
        // propertyName
        napi_value propertyNameValue;
        napi_create_string_utf8(env, node.propertyName.c_str(), NAPI_AUTO_LENGTH, &propertyNameValue);
        napi_set_named_property(env, jsNode, "propertyName", propertyNameValue);
        
        return jsNode;
    }
    
    // 将引用链转换为JavaScript数组
    napi_value CreateReferenceChain(napi_env env, const std::vector<ReferenceChainNode>& chain) {
        napi_value jsChain;
        napi_create_array_with_length(env, chain.size(), &jsChain);
        
        for (size_t i = 0; i < chain.size(); i++) {
            napi_value jsNode = CreateReferenceChainNode(env, chain[i]);
            napi_set_element(env, jsChain, i, jsNode);
        }
        
        return jsChain;
    }
    
    // 将结果转换为JavaScript对象
    napi_value CreateResultObject(napi_env env, 
                                const std::map<std::string, std::vector<std::vector<ReferenceChainNode>>>& result) {
        napi_value jsResult;
        napi_create_object(env, &jsResult);
        
        for (std::map<std::string, std::vector<std::vector<ReferenceChainNode>>>::const_iterator it = result.begin();
             it != result.end(); ++it) {
            const std::string& className = it->first;
            const std::vector<std::vector<ReferenceChainNode>>& chains = it->second;
            
            // 创建该类的引用链数组
            napi_value jsClassChains;
            napi_create_array_with_length(env, chains.size(), &jsClassChains);
            
            for (size_t i = 0; i < chains.size(); i++) {
                napi_value jsChain = CreateReferenceChain(env, chains[i]);
                napi_set_element(env, jsClassChains, i, jsChain);
            }
            
            // 将类名和引用链数组添加到结果对象中
            napi_value jsClassName;
            napi_create_string_utf8(env, className.c_str(), NAPI_AUTO_LENGTH, &jsClassName);
            napi_set_named_property(env, jsResult, className.c_str(), jsClassChains);
        }
        
        return jsResult;
    }
    
    // NAPI函数：解析堆快照并查找引用链
    napi_value ParseHeapSnapshotAndFindChains(napi_env env, napi_callback_info info) {
        size_t argc = 2;
        napi_value args[2];
        napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
        
        if (argc < 2) {
            napi_throw_error(env, nullptr, "Wrong number of arguments. Expected: filepath, classNames");
            napi_value undefined;
            napi_get_undefined(env, &undefined);
            return undefined;
        }
        
        // 获取文件路径参数
        size_t filePathLength;
        napi_get_value_string_utf8(env, args[0], nullptr, 0, &filePathLength);
        std::string filePath(filePathLength, '\0');
        napi_get_value_string_utf8(env, args[0], &filePath[0], filePathLength + 1, nullptr);
        
        // 获取类名数组参数
        uint32_t arrayLength;
        napi_get_array_length(env, args[1], &arrayLength);
        
        std::vector<std::string> classNames;
        for (uint32_t i = 0; i < arrayLength; i++) {
            napi_value jsElement;
            napi_get_element(env, args[1], i, &jsElement);
            
            size_t elementLength;
            napi_get_value_string_utf8(env, jsElement, nullptr, 0, &elementLength);
            std::string element(elementLength, '\0');
            napi_get_value_string_utf8(env, jsElement, &element[0], elementLength + 1, nullptr);
            
            classNames.push_back(element);
        }
        
        // 执行解析和查找引用链的操作
        HeapSnapshotParser parser;
        if (!parser.parseSnapshot(filePath)) {
            napi_throw_error(env, nullptr, "Failed to parse heap snapshot file");
            napi_value undefined;
            napi_get_undefined(env, &undefined);
            return undefined;
        }
        
        // 为每个类名查找引用链
        std::map<std::string, std::vector<std::vector<ReferenceChainNode>>> result = 
            parser.findReferenceChainsForMultipleClasses(classNames);
        
        // 将结果转换为JavaScript对象
        napi_value jsResult = CreateResultObject(env, result);
        return jsResult;
    }
}

// 模块初始化函数
static napi_value Init(napi_env env, napi_value exports) {
    // 定义模块的公共接口
    napi_property_descriptor desc[] = {
        {"parseHeapSnapshotAndFindChains", nullptr, ParseHeapSnapshotAndFindChains, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}


// 注册模块
static napi_module heap_snapshot_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "leakcanary",
    .nm_priv = ((void*)0),
    .reserved = {0}
};

// 模块注册入口
extern "C" __attribute__((constructor)) void RegisterHeapSnapshotModule(void) {
    napi_module_register(&heap_snapshot_module);
}