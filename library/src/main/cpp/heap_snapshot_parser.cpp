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
        std::regex nameRegex(R"((.*)\(line:(.*)\)(.*))");
        std::smatch match;
        
        if (std::regex_search(namePart, match, nameRegex)) {
            node.name = match[1];
            try {
                node.line = std::stoi(match[2]);
            } catch (...) {
                node.line = 0;
            }
            node.module = match[3];
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
std::vector<std::vector<ReferenceChain>> TaskHeapSnapshot::getShortestPathToGCRoot(int nodeId, int maxPaths) {
    std::vector<std::vector<ReferenceChain>> resultChains;
    
    // 查找目标节点
    auto it = nodeIdToIndexMap.find(nodeId);
    if (it == nodeIdToIndexMap.end()) {
        std::cerr << "未找到ID为 " << nodeId << " 的节点" << std::endl;
        return resultChains;
    }
    
    int targetNodeIndex = it->second;
    const HeapNode& targetNode = nodes[targetNodeIndex];
    
    // 如果目标节点本身就是GC根，返回空链
    if (isGCRoot(targetNode.type, targetNode.name)) {
        return resultChains;
    }
    
    // BFS队列：(当前节点索引, 路径)
    std::queue<std::pair<int, std::vector<ReferenceChain>>> queue;
    
    // 访问标记
    std::vector<bool> visited(nodes.size(), false);
    
    // 初始化队列
    queue.push({targetNodeIndex, {}});
    visited[targetNodeIndex] = true;
    
    // 最短路径长度，用于提前终止
    int shortestPathLength = -1;
    
    while (!queue.empty() && resultChains.size() < maxPaths) {
        auto [currentNodeIndex, currentPath] = queue.front();
        queue.pop();
        
        const HeapNode& currentNode = nodes[currentNodeIndex];
        
        // 检查当前节点是否是GC根
        if (isGCRoot(currentNode.type, currentNode.name)) {
            // 如果当前路径长度大于1（不是直接引用），则添加到结果
            if (currentPath.size() > 0) {
                resultChains.push_back(currentPath);
                if (shortestPathLength == -1) {
                    shortestPathLength = currentPath.size();
                }
            }
            continue;
        }
        
        // 如果已经找到足够的路径且当前路径长度超过最短路径，提前终止
        if (shortestPathLength != -1 && currentPath.size() > shortestPathLength) {
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
    
    return resultChains;
}

std::vector<std::vector<std::vector<ReferenceChain>>> TaskHeapSnapshot::getShortestPathToGCRootByName(const std::string& nodeName, int maxPaths) {
    std::vector<std::vector<std::vector<ReferenceChain>>> resultChains;
    
    // 查找所有名称匹配的节点
    std::vector<int> matchedNodeIds;
    for (const auto& node : nodes) {
        if (node.name.find(nodeName) != std::string::npos && node.type == "object") {
            matchedNodeIds.push_back(node.id);
        }
    }
    
    if (matchedNodeIds.empty()) {
        std::cerr << "未找到名称包含 \"" << nodeName << "\" 的节点" << std::endl;
        return resultChains;
    }
    
    // 对每个匹配的节点，获取其到GC根的最短路径
    for (int nodeId : matchedNodeIds) {
        std::vector<std::vector<ReferenceChain>> chains = getShortestPathToGCRoot(nodeId, maxPaths);
        resultChains.push_back(chains);
    }
    
    return resultChains;
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