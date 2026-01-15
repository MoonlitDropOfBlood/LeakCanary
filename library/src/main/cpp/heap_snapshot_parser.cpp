#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <queue>
#include <regex>
#include <memory>
// 替换jsoncpp为rapidjson，使用SAX方式处理大文件
#include "rapidjson/reader.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "heap_snapshot_parser.h"

// 定义GC根类型的检查
bool isGCRoot(const std::string& nodeType, const std::string& nodeName) {
    return nodeType == "synthetic" ||
           nodeType == "hidden" ||
           nodeName == "(GC root)" ||
           nodeName == "(root)" ||
           nodeName == "(global)" ||
           nodeName.find("(V8 internal)") == 0;
}

// 边类型映射函数
std::string mapEdgeType(int typeCode) {
    switch (typeCode) {
        case 0: return "context";
        case 1: return "element";
        case 2: return "property";
        case 3: return "internal";
        case 4: return "hidden";
        case 5: return "shortcut";
        case 6: return "weak";
        default: return "unknown";
    }
}

// 节点类型映射函数 ["hidden","array","string","object","code","closure","regexp","number","native","synthetic","concatenated string","slicedstring","symbol","bigint","framework"]
std::string mapNodeType(int typeCode) {
    switch (typeCode) {
        case 0: return "hidden";
        case 1: return "array";
        case 2: return "string";
        case 3: return "object";
        case 4: return "code";
        case 5: return "closure";
        case 6: return "regexp";
        case 7: return "number";
        case 8: return "native";
        case 9: return "synthetic";
        case 10: return "concatenated string";
        case 11: return "slicedstring";
        case 12: return "symbol";
        case 13: return "bigint";
        case 14: return "framework";
        default: return "unknown";
    }
}

// 解析节点名称和路径信息
void TaskHeapSnapshot::parseNodeNameAndPath(HeapNode& node, const std::string& originalName) {
    if (originalName.find("#") != std::string::npos && originalName[0] != '#' && originalName[0] != '=') {
        size_t hashPos = originalName.find("#");
        node.path = originalName.substr(0, hashPos);
        
        std::string namePart = originalName.substr(hashPos + 1);
        
        // 查找 "(line:" 和 ")" 的位置
        size_t lineStart = namePart.find("(line:");
        if (lineStart != std::string::npos) {
            size_t lineEnd = namePart.find(")", lineStart);
            if (lineEnd != std::string::npos) {
                node.name = namePart.substr(0, lineStart);
                
                // 提取行号部分并转换
                std::string lineStr = namePart.substr(lineStart + 6, lineEnd - lineStart - 6); // 6 是 "(line:" 的长度
                try {
                    node.line = std::stoi(lineStr);
                } catch (...) {
                    node.line = 0;
                }
                
                // 提取模块部分
                node.module = namePart.substr(lineEnd + 1);
            } else {
                node.name = namePart;
            }
        } else {
            node.name = namePart;
        }
    } else {
        node.name = originalName;
    }
}

// 获取字符串
std::string TaskHeapSnapshot::getStringById(int id) const {
    if (id >= 0 && id < static_cast<int>(strings.size())) {
        return strings[id];
    }
    return "";
}

// SAX解析处理器
class TaskHeapSnapshotHandler {
private:
    std::vector<std::string>& strings;
    std::vector<int>& nodesRaw;
    std::vector<int>& edgesRaw;
    TaskHeapSnapshot::Meta& meta;
    
    enum ParseState {
        None,
        InStrings,
        InNodes,
        InEdges,
        InMeta,
        InNodeFields,
        InNodeTypes,
        InEdgeFields,
        InEdgeTypes
    };
    
    ParseState currentState;
    int arrayIndex;
    
public:
    TaskHeapSnapshotHandler(std::vector<std::string>& s, std::vector<int>& nodesR, 
                          std::vector<int>& edgesR, TaskHeapSnapshot::Meta& m)
        : strings(s), nodesRaw(nodesR), edgesRaw(edgesR), meta(m),
          currentState(None), arrayIndex(0) {}
    
    // 获取字符串的辅助函数
    std::string getStringById(int id) const {
        if (id >= 0 && id < static_cast<int>(strings.size())) {
            return strings[id];
        }
        return "";
    }
    
    bool Null() { 
        if (currentState == InStrings && arrayIndex >= (int)strings.size()) {
            strings.push_back("null");
        }
        arrayIndex++;
        return true; 
    }
    
    bool Bool(bool b) { 
        arrayIndex++;
        return true; 
    }
    
    bool Int(int i) {
        switch (currentState) {
        case InStrings:
            if (arrayIndex >= (int)strings.size()) {
                strings.push_back(std::to_string(i));
            }
            break;
        case InNodes:
            nodesRaw.push_back(i);
            break;
        case InEdges:
            edgesRaw.push_back(i);
            break;
        case InNodeTypes:
            if (arrayIndex % 2 == 0) {
                // 类型名称
                meta.node_types.emplace_back();
                meta.node_types.back().push_back(getStringById(i));
            } else {
                // 类型值
                meta.node_types.back().push_back(std::to_string(i));
            }
            break;
        case InEdgeTypes:
            if (arrayIndex % 2 == 0) {
                // 类型名称
                meta.edge_types.emplace_back();
                meta.edge_types.back().push_back(getStringById(i));
            } else {
                // 类型值
                meta.edge_types.back().push_back(std::to_string(i));
            }
            break;
        }
        arrayIndex++;
        return true;
    }
    
    bool Uint(unsigned u) { 
        Int(static_cast<int>(u)); 
        return true; 
    }
    
    bool Int64(int64_t i) { 
        Int(static_cast<int>(i)); 
        return true; 
    } 
    
    bool Uint64(uint64_t u) { 
        Int(static_cast<int>(u)); 
        return true; 
    }
    
    bool Double(double d) { 
        arrayIndex++;
        return true; 
    }
    
    bool String(const char* str, rapidjson::SizeType length, bool copy) {
        switch (currentState) {
        case InStrings:
            if (arrayIndex >= (int)strings.size()) {
                strings.push_back(std::string(str, length));
            }
            break;
        case InNodeFields:
            meta.node_fields.push_back(std::string(str, length));
            break;
        case InEdgeFields:
            meta.edge_fields.push_back(std::string(str, length));
            break;
        }
        arrayIndex++;
        return true;
    }
    
    bool RawNumber(const char* str, rapidjson::SizeType length, bool copy) {
        if (length == 0) {
            arrayIndex++;
            return true;
        }
        
        std::string numStr(str, length);
        try {
            int value = std::stoi(numStr);
            switch (currentState) {
            case InNodes:
                nodesRaw.push_back(value);
                break;
            case InEdges:
                edgesRaw.push_back(value);
                break;
            }
        } catch (...) {
            // 忽略无法转换的数值
        }
        arrayIndex++;
        return true;
    }
    
    bool StartObject() { 
        if (currentState == None) {
            currentState = InMeta;
        }
        return true; 
    }
    
    bool Key(const char* str, rapidjson::SizeType length, bool copy) {
        std::string key(str, length);
        if (key == "strings") {
            currentState = InStrings;
            arrayIndex = 0;
        } else if (key == "nodes") {
            currentState = InNodes;
            arrayIndex = 0;
        } else if (key == "edges") {
            currentState = InEdges;
            arrayIndex = 0;
        } else if (key == "node_fields") {
            currentState = InNodeFields;
            arrayIndex = 0;
        } else if (key == "node_types") {
            currentState = InNodeTypes;
            arrayIndex = 0;
        } else if (key == "edge_fields") {
            currentState = InEdgeFields;
            arrayIndex = 0;
        } else if (key == "edge_types") {
            currentState = InEdgeTypes;
            arrayIndex = 0;
        } else if (key == "node_count") {
            currentState = None;
        } else if (key == "edge_count") {
            currentState = None;
        }
        return true;
    }
    
    bool EndObject(rapidjson::SizeType memberCount) { 
        if (currentState == InMeta) {
            currentState = None;
        }
        return true; 
    }
    
    bool StartArray() { 
        arrayIndex = 0;
        return true; 
    }
    
    bool EndArray(rapidjson::SizeType elementCount) { 
        if (currentState == InStrings) {
            strings.resize(elementCount);
        }
        currentState = None;
        return true; 
    }
};

// 解析快照文件
bool TaskHeapSnapshot::parseSnapshot() {
    // 使用文件流以更好地处理大文件
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) {
        std::cerr << "无法打开文件: " << path << std::endl;
        return false;
    }
    
    // 创建文件读取流，使用64KB缓冲区
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    // 创建SAX解析器
    rapidjson::Reader reader;
    TaskHeapSnapshotHandler handler(strings, nodesRaw, edgesRaw, meta);
    
    // 解析JSON
    rapidjson::ParseResult result = reader.Parse(is, handler);
    
    fclose(fp);
    
    if (!result) {
        std::cerr << "JSON解析失败: " << rapidjson::GetParseError_En(result.Code())
                  << " at offset " << result.Offset() << std::endl;
        return false;
    }
    
    // 解析完成后，处理元数据和节点边数据
    parseMetaAndData();
    
    // 构建引用关系
    buildReferences();
    
    return true;
}

// 解析元数据和节点边数据
void TaskHeapSnapshot::parseMetaAndData() {
    // 解析节点数据
    int nodeFieldsCount = meta.node_fields.size();
    for (int i = 0; i < nodesRaw.size(); i += nodeFieldsCount) {
        HeapNode node;
        
        for (int j = 0; j < nodeFieldsCount; j++) {
            const std::string& field = meta.node_fields[j];
            int value = nodesRaw[i + j];
            
            if (field == "type") {
                node.type = mapNodeType(value);
            } else if (field == "name") {
                std::string originalName = getStringById(value);
                parseNodeNameAndPath(node, originalName);
            } else if (field == "id") {
                node.id = value;
                nodeIdToIndexMap[value] = static_cast<int>(nodes.size());
            } else if (field == "edge_count") {
                node.edge_count = value;
            }
        }
        
        nodes.push_back(node);
    }
    
    // 解析边数据
    int edgeFieldsCount = meta.edge_fields.size();
    for (int i = 0; i < edgesRaw.size(); i += edgeFieldsCount) {
        std::string type;
        std::string name_or_index;
        int to_node = 0;
        
        for (int j = 0; j < edgeFieldsCount; j++) {
            const std::string& field = meta.edge_fields[j];
            int value = edgesRaw[i + j];
            
            if (field == "type") {
                type = mapEdgeType(value);
            } else if (field == "name_or_index") {
                // 检查是否为字符串索引
                if (type != "element") {
                    if (value < static_cast<int>(strings.size())) {
                        name_or_index = strings[value];
                    } else {
                        name_or_index = std::to_string(value);
                    }
                } else {
                    name_or_index = std::to_string(value);
                }
            } else if (field == "to_node") {
                // 转换为节点索引
                to_node = value / nodeFieldsCount;
            }
        }
        
        edges.emplace_back(type, name_or_index, to_node);
    }
}

// 构建引用关系
void TaskHeapSnapshot::buildReferences() {
    int edgeIndex = 0;
    for (size_t i = 0; i < nodes.size(); i++) {
        HeapNode& node = nodes[i];
        int edgeCount = node.edge_count;
        
        for (int j = 0; j < edgeCount; j++) {
            if (edgeIndex + j >= edges.size()) break;
            
            const Edge& edge = edges[edgeIndex + j];
            int toNodeIndex = edge.to_node_index;
            
            if (toNodeIndex >= 0 && toNodeIndex < static_cast<int>(nodes.size())) {
                HeapNode& toNode = nodes[toNodeIndex];
                toNode.references.emplace_back(static_cast<int>(i), edge.type, edge.name_or_index);
            }
        }
        
        edgeIndex += edgeCount;
    }
}

// 使用BFS算法查找从目标节点到GC根的最短引用链
std::vector<ReferenceChain> TaskHeapSnapshot::getShortestPathToGCRoot(int nodeId, int maxDepth) {
    std::vector<ReferenceChain> shortestChain;
    
    // 查找目标节点
    auto it = nodeIdToIndexMap.find(nodeId);
    if (it == nodeIdToIndexMap.end()) {
        std::cerr << "未找到ID为 " << nodeId << " 的节点" << std::endl;
        return shortestChain;
    }
    
    int targetNodeIndex = it->second;
    const HeapNode& targetNode = nodes[targetNodeIndex];
    
    // 如果目标节点本身就是GC根，返回空链
    if (isGCRoot(targetNode.type, targetNode.name)) {
        return shortestChain;
    }
    
    // BFS队列：(当前节点索引, 路径)
    std::queue<std::pair<int, std::vector<ReferenceChain>>> queue;
    
    // 访问标记
    std::vector<bool> visited(nodes.size(), false);
    
    // 初始化队列
    queue.push({targetNodeIndex, {}});
    visited[targetNodeIndex] = true;
    
    // 最短路径长度标记
    bool foundShortest = false;
    
    while (!queue.empty()) {
        auto [currentNodeIndex, currentPath] = queue.front();
        queue.pop();
        
        // 检查是否超过最大深度
        if (currentPath.size() >= maxDepth) {
            continue;
        }
        
        const HeapNode& currentNode = nodes[currentNodeIndex];
        
        // 检查当前节点是否是GC根
        if (isGCRoot(currentNode.type, currentNode.name)) {
            // 如果找到GC根且路径非空，这就是最短路径
            if (!currentPath.empty()) {
                shortestChain = currentPath;
                foundShortest = true;
                break; // 找到第一条最短路径后立即返回
            }
            continue;
        }
        
        // 遍历当前节点的所有引用者
        for (const Reference& ref : currentNode.references) {
            int referrerIndex = ref.from_node_index;
            
            // 跳过已访问的节点
            if (visited[referrerIndex]) {
                continue;
            }
            
            const HeapNode& referrer = nodes[referrerIndex];
            
            // 跳过直接的GC根引用
            if (isGCRoot(referrer.type, referrer.name) && currentPath.empty()) {
                continue;
            }
            // 跳过弱引用
            if (ref.type == "weak") {
                continue;
            }
                
            // 创建新的引用链节点
            ReferenceChainNode referrerNode(referrer.id, referrer.name, referrer.type, 
                                           referrer.path, referrer.line);
            
            ReferenceChainNode currentNodeRef(currentNode.id, currentNode.name, currentNode.type, 
                                             currentNode.path, currentNode.line);
            
            // 创建新的引用链
            ReferenceChain newChain(referrerNode, ref.type, currentNodeRef);
            
            // 创建新的路径
            std::vector<ReferenceChain> newPath = currentPath;
            newPath.push_back(newChain);
            
            // 添加到队列
            queue.push({referrerIndex, newPath});
            visited[referrerIndex] = true;
        }
    }
    
    return shortestChain;
}

HeapNode* TaskHeapSnapshot::findHashNodeByName(const std::string& targetName) {
    auto it = std::find_if(nodes.begin(), nodes.end(),
        [&targetName](const HeapNode& node) {
            return node.name == targetName && node.type == "number";
        });
    
    if (it != nodes.end()) {
        // 获取该节点的引用列表
        const std::vector<Reference>& refs = it->references;
        
        // 在引用列表中查找 name_or_index 为 "ArkInternalHash" 的引用
        auto refIt = std::find_if(refs.begin(), refs.end(),
            [](const Reference& ref) {
                return ref.name_or_index == "ArkInternalHash";
            });
        
        if (refIt != refs.end()) {
            // 找到目标引用，返回引用指向的节点
            int targetNodeIndex = refIt->from_node_index;
            if (targetNodeIndex >= 0 && targetNodeIndex < static_cast<int>(nodes.size())) {
                return &nodes[targetNodeIndex];
            }
        }
    }
    return nullptr;
}

std::vector<ReferenceChain> TaskHeapSnapshot::getShortestPathToGCRootByName(const std::string& nodeName, int maxDepth) {
    
    // 查找所有名称匹配的节点
    const HeapNode* node = findHashNodeByName(nodeName);
  
    if (node == nullptr) {
        std::cerr << "未找到名称包含 \"" << nodeName << "\" 的节点" << std::endl;
        return std::vector<ReferenceChain>();
    }
    std::vector<ReferenceChain> chain = getShortestPathToGCRoot(node->id,maxDepth);
    return chain;
}

// 初始化TaskManager静态成员
std::map<int, std::unique_ptr<TaskHeapSnapshot>> TaskManager::tasks;
int TaskManager::nextTaskId = 1;

// 创建新任务
int TaskManager::createTask(const std::string& path) {
    int taskId = nextTaskId++;
    auto task = std::make_unique<TaskHeapSnapshot>(taskId, path);
    
    // 解析快照文件
    if (task->parseSnapshot()) {
        tasks[taskId] = std::move(task);
        return taskId;
    } else {
        return -1; // 创建失败
    }
}

// 获取任务
TaskHeapSnapshot* TaskManager::getTask(int id) {
    auto it = tasks.find(id);
    if (it != tasks.end()) {
        return it->second.get();
    }
    return nullptr;
}

// 销毁任务
bool TaskManager::destroyTask(int id) {
    auto it = tasks.find(id);
    if (it != tasks.end()) {
        tasks.erase(it);
        return true;
    }
    return false;
}