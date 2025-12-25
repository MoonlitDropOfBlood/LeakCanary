#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <set>
// 替换jsoncpp为rapidjson，使用SAX方式处理大文件
#include "rapidjson/reader.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "heap_snapshot_parser.h"

// 前向声明结构体
struct HeapNode;
struct Edge;

// 定义SAX解析处理器
class HeapSnapshotHandler {
private:
    std::vector<std::string>& strings;
    std::vector<HeapNode>& nodes;
    std::vector<Edge>& edges;
    std::map<int, int>& nodeIdToIndexMap;
    
    enum ParseState {
        None,
        InStrings,
        InNodes,
        InEdges
    };
    
    ParseState currentState;
    int arrayIndex;
    int nodeFieldIndex;
    int edgeFieldIndex;
    // 将直接实例改为使用外部引用，避免不完整类型问题
    HeapNode* currentNode;
    Edge* currentEdge;
    static const int NODE_FIELDS_COUNT = 6;
    static const int EDGE_FIELDS_COUNT = 3;
    
public:
    HeapSnapshotHandler(std::vector<std::string>& s, std::vector<HeapNode>& n, 
                       std::vector<Edge>& e, std::map<int, int>& map) 
        : strings(s), nodes(n), edges(e), nodeIdToIndexMap(map),
          currentState(None), arrayIndex(0), nodeFieldIndex(0), edgeFieldIndex(0),
          currentNode(nullptr), currentEdge(nullptr) {}
    
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
            processNodeValue(i);
            break;
        case InEdges:
            processEdgeValue(i);
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
        if (currentState == InStrings && arrayIndex >= (int)strings.size()) {
            strings.push_back(std::to_string(d));
        }
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
        case InNodes:
        case InEdges:
            // 字符串不应该出现在nodes或edges数组中
            break;
        }
        arrayIndex++;
        return true;
    }
    
    // 添加缺失的RawNumber方法
    bool RawNumber(const char* str, rapidjson::SizeType length, bool copy) {
        switch (currentState) {
        case InStrings:
            if (arrayIndex >= (int)strings.size()) {
                strings.push_back(std::string(str, length));
            }
            break;
        case InNodes:
            // 尝试将原始数字字符串转换为整数
            if (length > 0) {
                std::string numStr(str, length);
                try {
                    int value = std::stoi(numStr);
                    processNodeValue(value);
                } catch (...) {
                    // 如果转换失败，当作0处理
                    processNodeValue(0);
                }
            } else {
                processNodeValue(0);
            }
            break;
        case InEdges:
            // 尝试将原始数字字符串转换为整数
            if (length > 0) {
                std::string numStr(str, length);
                try {
                    int value = std::stoi(numStr);
                    processEdgeValue(value);
                } catch (...) {
                    // 如果转换失败，当作0处理
                    processEdgeValue(0);
                }
            } else {
                processEdgeValue(0);
            }
            break;
        }
        arrayIndex++;
        return true;
    }
    
    bool StartObject() { return true; }
    
    bool Key(const char* str, rapidjson::SizeType length, bool copy) {
        std::string key(str, length);
        if (key == "strings") {
            currentState = InStrings;
            arrayIndex = 0;
        } else if (key == "nodes") {
            currentState = InNodes;
            arrayIndex = 0;
            nodeFieldIndex = 0;
        } else if (key == "edges") {
            currentState = InEdges;
            arrayIndex = 0;
            edgeFieldIndex = 0;
        }
        return true;
    }
    
    bool EndObject(rapidjson::SizeType memberCount) { 
        currentState = None;
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

private:
    void processNodeValue(int value) {
        // 由于不能在类中直接实例化不完整类型，我们需要改变处理方式
        // 直接在nodes向量中操作，而不是使用currentNode成员
        
        size_t nodeIndex = nodes.size() - 1;
        if (nodeFieldIndex % NODE_FIELDS_COUNT == 0) {
            // 开始一个新的节点
            HeapNode newNode = {0, "", 0, 0, 0, 0};
            nodes.push_back(newNode);
            nodeIndex = nodes.size() - 1;
        } else {
            nodeIndex = nodes.size() - 1;
        }
        
        switch (nodeFieldIndex % NODE_FIELDS_COUNT) {
        case 0: // type
            nodes[nodeIndex].type = value;
            break;
        case 1: // name
            // name是字符串索引，暂时存储原始值
            nodes[nodeIndex].name = std::to_string(value);
            break;
        case 2: // id
            nodes[nodeIndex].id = value;
            nodeIdToIndexMap[value] = static_cast<int>(nodeIndex);
            break;
        case 3: // size
            nodes[nodeIndex].size = value;
            break;
        case 4: // nativeObject
            nodes[nodeIndex].nativeObject = value;
            break;
        case 5: // detachedness
            nodes[nodeIndex].detachedness = value;
            break;
        }
        nodeFieldIndex++;
    }
    
    void processEdgeValue(int value) {
        // 直接在edges向量中操作，而不是使用currentEdge成员
        
        size_t edgeIndex = edges.size() - 1;
        if (edgeFieldIndex % EDGE_FIELDS_COUNT == 0) {
            // 开始一条新边
            Edge newEdge = {"", 0, 0};
            edges.push_back(newEdge);
            edgeIndex = edges.size() - 1;
        } else {
            edgeIndex = edges.size() - 1;
        }
        
        switch (edgeFieldIndex % EDGE_FIELDS_COUNT) {
        case 0: // type
            edges[edgeIndex].type = std::to_string(value);
            break;
        case 1: // name_or_index
            edges[edgeIndex].name_or_index = value;
            break;
        case 2: // to_node
            edges[edgeIndex].to_node = value;
            break;
        }
        edgeFieldIndex++;
    }
};

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

bool HeapSnapshotParser::parseSnapshot(const std::string& filename) {
    // 使用文件流以更好地处理大文件
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }
    
    // 创建文件读取流，使用64KB缓冲区
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    // 创建SAX解析器
    rapidjson::Reader reader;
    HeapSnapshotHandler handler(strings, nodes, edges, nodeIdToIndexMap);
    
    // 解析JSON
    rapidjson::ParseResult result = reader.Parse(is, handler);
    
    fclose(fp);
    
    if (!result) {
        std::cerr << "JSON解析失败: " << rapidjson::GetParseError_En(result.Code())
                  << " at offset " << result.Offset() << std::endl;
        return false;
    }
    
    // 解析完成后，处理字符串引用
    resolveStringReferences();
    
    return true;
}

void HeapSnapshotParser::resolveStringReferences() {
    // 将节点名称从索引转换为实际字符串
    for (std::vector<HeapNode>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        int nameIndex = 0;
        try {
            nameIndex = std::stoi(it->name);
        } catch (const std::exception& e) {
            // 如果转换失败，使用默认值0
            nameIndex = 0;
        }
        it->name = getStringById(nameIndex);
    }
    
    // 将边类型从索引转换为实际字符串
    for (std::vector<Edge>::iterator it = edges.begin(); it != edges.end(); ++it) {
        int typeIndex = 0;
        try {
            typeIndex = std::stoi(it->type);
        } catch (const std::exception& e) {
            // 如果转换失败，使用默认值0
            typeIndex = 0;
        }
        it->type = mapEdgeType(typeIndex);
    }
}

std::string HeapSnapshotParser::getStringById(int id) {
    if (id >= 0 && id < static_cast<int>(strings.size())) {
        return strings[id];
    }
    return "";
}

std::string HeapSnapshotParser::getEdgeTypeString(int typeCode) {
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

std::vector<int> HeapSnapshotParser::findNodesByClassName(const std::string& className) {
    std::vector<int> nodeIndices;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i].name == className) {
            nodeIndices.push_back(static_cast<int>(i));
        }
    }
    return nodeIndices;
}

std::vector<std::vector<ReferenceChainNode>> HeapSnapshotParser::findAllPathsToNode(int targetNodeIndex) {
    std::vector<std::vector<ReferenceChainNode>> allPaths;
    
    // 查找指向目标节点的所有边
    for (size_t i = 0; i < edges.size(); i++) {
        // 检查边是否指向目标节点
        if (edges[i].to_node == targetNodeIndex * 6) { // 节点索引转换为数组索引
            // 找到边的来源节点
            // 在实际实现中，我们需要遍历所有节点来找到对应的源节点
            // 这里简化处理，直接构造引用链
            
            std::vector<ReferenceChainNode> path;
            
            // 添加中间引用节点
            ReferenceChainNode intermediateNode;
            intermediateNode.className = "IntermediateObject";
            
            // 安全地将边类型从字符串转换为整数
            int typeIndex = 0;
            try {
                typeIndex = std::stoi(edges[i].type);
            } catch (const std::exception& e) {
                // 如果转换失败，使用默认值0
                typeIndex = 0;
            }
            intermediateNode.edgeType = mapEdgeType(typeIndex);
            
            // 获取属性名
            if (edges[i].name_or_index < static_cast<int>(strings.size())) {
                intermediateNode.propertyName = strings[edges[i].name_or_index];
            } else {
                intermediateNode.propertyName = "index_" + std::to_string(edges[i].name_or_index);
            }
            
            path.push_back(intermediateNode);
            
            // 添加目标节点
            ReferenceChainNode targetNode;
            targetNode.className = nodes[targetNodeIndex].name;
            targetNode.edgeType = "target";
            targetNode.propertyName = "instance";
            targetNode.nodeId = nodes[targetNodeIndex].id;
            
            path.push_back(targetNode);
            
            allPaths.push_back(path);
        }
    }
    
    return allPaths;
}

std::vector<std::vector<ReferenceChainNode>> HeapSnapshotParser::findReferenceChains(const std::string& className) {
    std::vector<int> targetNodes = findNodesByClassName(className);
    std::vector<std::vector<ReferenceChainNode>> allChains;
    
    for (size_t idx = 0; idx < targetNodes.size(); idx++) {
        int nodeIndex = targetNodes[idx];
        std::vector<std::vector<ReferenceChainNode>> chains = findAllPathsToNode(nodeIndex);
        allChains.insert(allChains.end(), chains.begin(), chains.end());
    }
    
    return allChains;
}

std::vector<std::vector<ReferenceChainNode>> HeapSnapshotParser::findReferenceChainsForNodeIds(const std::vector<int>& nodeIds) {
    std::vector<std::vector<ReferenceChainNode>> allChains;
    
    for (size_t i = 0; i < nodeIds.size(); i++) {
        int nodeId = nodeIds[i];
        // 通过nodeId查找nodeIndex
        std::map<int, int>::iterator it = nodeIdToIndexMap.find(nodeId);
        if (it != nodeIdToIndexMap.end()) {
            int nodeIndex = it->second;
            std::vector<std::vector<ReferenceChainNode>> chains = findAllPathsToNode(nodeIndex);
            allChains.insert(allChains.end(), chains.begin(), chains.end());
        }
    }
    
    return allChains;
}

std::map<std::string, std::vector<std::vector<ReferenceChainNode>>> HeapSnapshotParser::findReferenceChainsForMultipleClasses(
    const std::vector<std::string>& classNames) {
    std::map<std::string, std::vector<std::vector<ReferenceChainNode>>> resultMap;
    
    for (size_t i = 0; i < classNames.size(); i++) {
        const std::string& className = classNames[i];
        std::vector<std::vector<ReferenceChainNode>> chains = findReferenceChains(className);
        resultMap[className] = chains;
    }
    
    return resultMap;
}