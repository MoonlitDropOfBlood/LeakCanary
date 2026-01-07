import fs from 'node:fs'

/**
 * 读取heapData.heapsnapshot.json文件
 */
function readHeapData() {
    return JSON.parse(fs.readFileSync('../heapData.heapsnapshot.json', 'utf-8'))
}

interface Meta {
    node_fields: string[]
    node_types: Array<string | string[]>
    edge_fields: string[]
    edge_types: Array<string | string[]>
}

interface Snapshot {
    meta: Meta
    node_count: number
    edge_count: number
}

interface HeapData {
    snapshot: Snapshot
    nodes: number[]
    strings: string[]
    edges: number[]
    trace_trees: number[]
    trace_function_info: number[]
    samples: number[]
    locations: number[]
}

const heapData: HeapData = readHeapData()

const nodes: any[] = []
for (let i = 0; i < heapData.snapshot.node_count; i++) {
    const node: any = {}
    for (let j = 0; j < heapData.snapshot.meta.node_fields.length; j++) {
        const field = heapData.snapshot.meta.node_fields[j]
        const type = heapData.snapshot.meta.node_types[j]
        const value = heapData.nodes[(i * heapData.snapshot.meta.node_fields.length) + j]
        if (type === 'string') {
            node[field] = heapData.strings[value]
        } else if (typeof type == 'object') {
            node[field] = type[value]
        } else if (type == 'number' || type == undefined) {
            node[field] = value
        }
    }
    if (node.name.includes("#") && node.name.charAt(0) != '#' && node.name.charAt(0) != '=') {
        let name: string[] = node.name.split("#")
        node['path'] = name[0]
        //Custom1Component(line:7)[entry]
        //通过正则提前line:7中的7 和 [entry]中的  entry 和 开头括号之前的部分
        let reg = /(.*)\(line:(.*)\)(.*)/
        let result = reg.exec(name[1])
        if (result != null) {
            node['name'] = result[1]
            node['line'] = parseInt(result[2])
            node['module'] = result[3]
        }
    }
    nodes.push(node)
}
//输出nodes到文件
fs.writeFileSync('../nodes.json', JSON.stringify(nodes, null, 2))
const edges: any[] = []
for (let i = 0; i < heapData.edges.length; i += heapData.snapshot.meta.edge_fields.length) {
    const edge: any = {}
    for (let j = 0; j < heapData.snapshot.meta.edge_fields.length; j++) {
        const field = heapData.snapshot.meta.edge_fields[j]
        const type = heapData.snapshot.meta.edge_types[j]
        const value = heapData.edges[i + j]
        if (typeof type != 'string') {
            edge[field] = type[value]
        } else if (type == 'node') {
            edge[field] = nodes[value / heapData.snapshot.meta.node_fields.length]
        } else if (edge['type'] != 'element') {
            edge[field] = heapData.strings[value]
        } else {
            edge[field] = value
        }
    }
    edges.push(edge)
}


// 修正后的引用分析函数
function analyzeReferencesCorrectly() {
    let edgeIndex = 0
    nodes.forEach((node, index) => {
        const edge_count = node['edge_count']
        const fields = edges.slice(edgeIndex, edgeIndex + edge_count)
        node['fields'] = fields
        fields.forEach(field => {
            let newEdge = {
                ...field,
                from_node: node,
            }
            delete newEdge['to_node']
            if (field.to_node['references']) {
                field.to_node['references'].push(newEdge)
            } else {
                field.to_node['references'] = [newEdge]
            }
        })
        edgeIndex += edge_count
    })
}

// 执行分析
analyzeReferencesCorrectly()

// 使用广度优先搜索(BFS)来找到到GC根的最短路径 - 排除GC直接引用
function buildShortestReferenceChainToGCRoot(node: any) {
    // GC根通常具有特定的特征，如native类型或特殊的name
    const isGCRoot = (n: any) => {
        // 检查是否为GC根 - 通常是native类型或具有特定名称的节点
        return n.type === 'synthetic' ||
            n.type === 'hidden' ||
            n.name === '(GC root)' ||
            n.name === '(root)' ||
            n.name === '(global)' ||
            n.name.startsWith('(V8 internal)');
    };

    // 如果当前节点本身就是GC根，返回空链
    if (isGCRoot(node)) {
        return [];
    }

    // 使用BFS来寻找最短路径
    const queue: Array<{node: any, path: any[]}> = [{node: node, path: []}];
    const visited = new Set();

    // 记录已找到的GC根路径，避免重复处理
    let foundGCRootPath: any[] | null = null;

    while (queue.length > 0) {
        const {node: currentNode, path} = queue.shift()!;

        // 避免循环引用
        if (visited.has(currentNode.id)) {
            continue;
        }

        visited.add(currentNode.id);

        // 检查当前节点是否是GC根
        if (isGCRoot(currentNode)) {
            // 如果当前路径长度大于1，说明不是直接引用
            if (path.length > 0) {
                return path; // 返回找到的最短路径（排除直接引用）
            } else {
                // 如果是直接引用，继续搜索其他路径
                continue;
            }
        }

        // 如果当前节点有引用者，将它们加入队列
        if (currentNode.references) {
            for (const ref of currentNode.references) {
                const referrer = ref.from_node;

                // 检查引用者是否为GC根，如果是则跳过这条路径（排除直接引用）
                if (isGCRoot(referrer) && path.length === 0) {
                    console.log(`跳过GC直接引用路径: ${referrer.name} -> ${currentNode.name}`);
                    continue;
                }

                // 创建新的路径记录
                const newPath = [...path, {
                    referrer: {
                        id: referrer.id,
                        name: referrer.name,
                        type: referrer.type,
                        path: referrer.path,
                        line: referrer.line,
                    },
                    edge_type: ref.type,
                    current_node: {
                        id: currentNode.id,
                        name: currentNode.name,
                        type: currentNode.type,
                        path: referrer.path,
                        line: referrer.line,
                    }
                }];

                queue.push({node: referrer, path: newPath});
            }
        }
    }

    // 如果没有找到GC根的路径（排除直接引用的情况）
    return null;
}

function analyzeReferencesById(id: number) {
    const targetNode = nodes.find(node => node.id === id)

    if (targetNode) {
        console.log('目标节点信息:', targetNode)

        if (targetNode.references) {
            console.log('引用此节点的对象:', targetNode.references)

            // 获取到GC根的最短引用链
            const shortestChain = buildShortestReferenceChainToGCRoot(targetNode)

            if (shortestChain) {
                console.log('到GC根的最短引用链:', shortestChain);

                // 打印链路摘要
                console.log('引用链摘要:');
                shortestChain.forEach((link, index) => {
                    console.log(`${index + 1}. ${link.referrer.name} (${link.referrer.type}) -> ${link.current_node.name} (${link.edge_type})`);
                });
            } else {
                console.log('未找到到GC根的引用链，该对象可能不是内存泄漏的根源');
            }
        } else {
            console.log('该节点没有被其他对象引用');
        }
    } else {
        console.log('未找到 ID 为 ' + id + ' 的节点');
    }
}

analyzeReferencesById(12761)
