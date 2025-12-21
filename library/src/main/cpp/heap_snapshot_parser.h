#ifndef HEAP_SNAPSHOT_PARSER_H
#define HEAP_SNAPSHOT_PARSER_H

#include <string>
#include <vector>
#include <map>

struct ReferenceChainNode {
    std::string className;
    std::string edgeType;
    std::string propertyName;
};

struct HeapNode {
    int type;
    std::string name;
    int id;
    int size;
    int nativeObject;
    int detachedness;
};

struct Edge {
    std::string type;
    int name_or_index;
    int to_node;
};

class HeapSnapshotParser {
private:
    std::vector<std::string> strings;
    std::vector<int> nodesRaw;
    std::vector<int> edgesRaw;
    std::vector<HeapNode> nodes;
    std::vector<Edge> edges;
    std::map<int, int> nodeIdToIndexMap;

public:
    bool parseSnapshot(const std::string& filename);
    std::vector<std::vector<ReferenceChainNode>> findReferenceChains(const std::string& className);
    std::map<std::string, std::vector<std::vector<ReferenceChainNode>>> findReferenceChainsForMultipleClasses(
        const std::vector<std::string>& classNames);

private:
    void resolveStringReferences();
    std::string getEdgeTypeString(int typeCode);
    std::vector<int> findNodesByClassName(const std::string& className);
    std::vector<std::vector<ReferenceChainNode>> findAllPathsToNode(int targetNodeIndex);
    std::string getStringById(int id);
};

// 边类型映射函数
std::string mapEdgeType(int typeCode);

#endif // HEAP_SNAPSHOT_PARSER_H