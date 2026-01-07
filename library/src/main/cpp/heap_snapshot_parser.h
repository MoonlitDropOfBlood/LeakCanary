#ifndef HEAP_SNAPSHOT_PARSER_H
#define HEAP_SNAPSHOT_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

// 定义GC根类型的检查
bool isGCRoot(const std::string& nodeType, const std::string& nodeName);

// 边类型映射函数
std::string mapEdgeType(int typeCode);

// 节点类型映射函数
std::string mapNodeType(int typeCode);



// 引用链节点
struct ReferenceChainNode {
    int id;
    std::string name;
    std::string type;
    std::string path;
    int line;
    
    ReferenceChainNode() : id(0), line(0) {}
    ReferenceChainNode(int id_, const std::string& name_, const std::string& type_, 
                       const std::string& path_ = "", int line_ = 0)
        : id(id_), name(name_), type(type_), path(path_), line(line_) {}
};

// 引用链
struct ReferenceChain {
    ReferenceChainNode referrer;
    std::string edge_type;
    ReferenceChainNode current_node;
    
    ReferenceChain(const ReferenceChainNode& referrer_, const std::string& edge_type_, 
                   const ReferenceChainNode& current_node_)
        : referrer(referrer_), edge_type(edge_type_), current_node(current_node_) {}
};

// 节点引用
struct Reference {
    int from_node_index;
    std::string type;
    std::string name_or_index;
    
    Reference(int from_node_index_, const std::string& type_, const std::string& name_or_index_)
        : from_node_index(from_node_index_), type(type_), name_or_index(name_or_index_) {}
};

// 堆节点
struct HeapNode {
    int id;
    std::string name;
    std::string type;
    std::string path;
    int line;
    std::string module;
    int edge_count;
    std::vector<Reference> references;
    
    HeapNode() : id(0), line(0), edge_count(0) {}
};

// 边
struct Edge {
    std::string type;
    std::string name_or_index;
    int to_node_index;
    
    Edge(const std::string& type_, const std::string& name_or_index_, int to_node_index_)
        : type(type_), name_or_index(name_or_index_), to_node_index(to_node_index_) {}
};

// 任务堆快照
class TaskHeapSnapshot {
private:
    int id;
    std::string path;
    std::vector<std::string> strings;
    std::vector<HeapNode> nodes;
    std::vector<Edge> edges;
    std::map<int, int> nodeIdToIndexMap;
    
public:
    TaskHeapSnapshot(int id_, const std::string& path_)
        : id(id_), path(path_) {}
    
    ~TaskHeapSnapshot() = default;
    
    bool parseSnapshot();
    std::vector<std::vector<ReferenceChain>> getShortestPathToGCRoot(int nodeId, int maxPaths = 3);
    std::vector<std::vector<std::vector<ReferenceChain>>> getShortestPathToGCRootByName(const std::string& nodeName, int maxPaths = 3);
    
private:
    void parseMetaAndData();
    void buildReferences();
    std::string getStringById(int id) const;
    void parseNodeNameAndPath(HeapNode& node, const std::string& originalName);
    
    // 内部数据结构
    std::vector<int> nodesRaw;
    std::vector<int> edgesRaw;
    struct Meta {
        std::vector<std::string> node_fields;
        std::vector<std::string> edge_fields;
        std::vector<std::vector<std::string>> node_types;
        std::vector<std::vector<std::string>> edge_types;
        int node_count;
        int edge_count;
        
        Meta() : node_count(0), edge_count(0) {}
    } meta;
    
    // 声明友元类，允许TaskHeapSnapshotHandler访问私有成员
    friend class TaskHeapSnapshotHandler;
};

// 全局任务管理
class TaskManager {
private:
    static std::map<int, std::unique_ptr<TaskHeapSnapshot>> tasks;
    static int nextTaskId;
    
public:
    static int createTask(const std::string& path);
    static TaskHeapSnapshot* getTask(int id);
    static bool destroyTask(int id);
};

#endif // HEAP_SNAPSHOT_PARSER_H