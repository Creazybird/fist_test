#include<cstdio>
#include<metis.h>
#include<vector>
#include<cstdlib>
#include<memory.h>
#include<unordered_map>
#include<map>
#include<set>
#include<deque>
#include<stack>
#include<algorithm>
#include<sys/time.h>
#include<queue>
#include<unordered_set>
#include<cfloat>
#include<cstdint>
#include<climits>
#include <iostream>

using namespace std;

const char *FILE_NODE = "";
const char *FILE_EDGE = "";

const char *FILE_NODES_GTREE_PATH = "";
const char *FILE_GTREE = "";
const char *FILE_ONTREE_MIND = "";

const char *FILE_QUERY = "";
const char *FILE_OBJECT = "";

const char *FILE_SHORTCUT = "";

// MACRO for timing
struct timeval tv;
long double ts, te;
#define TIME_TICK_START gettimeofday( &tv, NULL ); ts = (long double) tv.tv_sec * 1000.0 + (long double) tv.tv_usec / 1000.0;
#define TIME_TICK_END gettimeofday( &tv, NULL ); te = (long double) tv.tv_sec * 1000.0 + (long double) tv.tv_usec / 1000.0;
// ----------

struct Node {
    int deep;
    int inleaf;
    int inleafpos;
    vector<int> adjnodes;
    vector<int> adjweight;
    vector<int> gtreepath; // this is used to do sub-graph locating
};

struct TreeNode {
    vector<int> borders;
    vector<int> children;
    bool isleaf;
    vector<int> leafnodes;
    int father;
// ----- distance matrices -----
    vector<int> union_borders; // for non leaf node
    vector<int> mind; // min dis, row by row of union_borders
// ----- for knn and range query-----
    unordered_set<int> oclist;
    bool is_visited;
// ----- for locating distance values in matrices
    vector<int> up_pos;
    vector<int> current_pos;
// ----- for caching distances
    vector<int> cache;
};

int noe; // number of edges
vector<Node> Nodes;
vector<TreeNode> GTree;

unordered_map<int, unordered_map<int, vector<int> > > shortcuts;
vector<int> query_objects;

void load_graph() {
    FILE *fin;

    fin = fopen(FILE_NODE, "r");
    char temp[50];
    char flag;
    int res;
    long count;

    do {
        res = fscanf(fin, "%c %s %s %s %ld", &flag, temp, temp, temp, &count);
    } while (res != 5);

    fclose(fin);

    Nodes.resize(count);

    fin = fopen(FILE_EDGE, "r");
    int snid, enid;
    int weight;
    noe = 0;

    do {
        res = fscanf(fin, "%c %d %d %d", &flag, &snid, &enid, &weight);
        if (res == 4 && flag == 'a') {
            noe++;
            Nodes[snid - 1].adjnodes.push_back(enid - 1);
            Nodes[snid - 1].adjweight.push_back(weight);
            Nodes[enid - 1].adjnodes.push_back(snid - 1);
            Nodes[enid - 1].adjweight.push_back(weight);
        }
    } while (res != EOF);
    fclose(fin);
}

// load gtree index from file
void load_gtree() {
    // FILE_GTREE
    FILE *fin = fopen(FILE_GTREE, "rb");
    int *buf = new int[Nodes.size()];
    int count_borders, count_children, count_leafnodes;
    bool isleaf;
    int father;

    // clear gtree
    GTree.clear();

    int node_count = 0;

    while (fread(&count_borders, sizeof(int), 1, fin)) {
        TreeNode tn;
        // borders
        tn.borders.clear();
        fread(buf, sizeof(int), count_borders, fin);
        for (int i = 0; i < count_borders; i++) {
            tn.borders.push_back(buf[i]);
        }
        // children
        fread(&count_children, sizeof(int), 1, fin);
        fread(buf, sizeof(int), count_children, fin);
        for (int i = 0; i < count_children; i++) {
            tn.children.push_back(buf[i]);
        }
        // isleaf
        fread(&isleaf, sizeof(bool), 1, fin);
        tn.isleaf = isleaf;
        // leafnodes
        fread(&count_leafnodes, sizeof(int), 1, fin);
        fread(buf, sizeof(int), count_leafnodes, fin);
        for (int i = 0; i < count_leafnodes; i++) {
            tn.leafnodes.push_back(buf[i]);
            Nodes[buf[i]].inleafpos = i;
        }
        // father
        fread(&father, sizeof(int), 1, fin);
        tn.father = father;

        GTree.push_back(tn);
    }
    fclose(fin);

    // FILE_NODES_GTREE_PATH
    int count;
    fin = fopen(FILE_NODES_GTREE_PATH, "rb");
    int pos = 0;
    while (fread(&count, sizeof(int), 1, fin)) {
        fread(buf, sizeof(int), count, fin);
        // clear gtreepath
        Nodes[pos].gtreepath.clear();
        for (int i = 0; i < count; i++) {
            Nodes[pos].gtreepath.push_back(buf[i]);
        }
        Nodes[pos].deep = count;
        Nodes[pos].inleaf = buf[count - 1];
        // pos increase
        pos++;
    }
    fclose(fin);
    delete[] buf;
}

// load distance matrix from file
void load_minds() {
    FILE *fin = fopen(FILE_ONTREE_MIND, "rb");
    int *buf;
    int count, pos = 0;
    while (fread(&count, sizeof(int), 1, fin)) {
        // union borders
        buf = new int[count];
        fread(buf, sizeof(int), count, fin);
        GTree[pos].union_borders.clear();
        for (int i = 0; i < count; i++) {
            GTree[pos].union_borders.push_back(buf[i]);
        }
        delete[] buf;
        // mind
        fread(&count, sizeof(int), 1, fin);
        buf = new int[count];
        fread(buf, sizeof(int), count, fin);
        GTree[pos].mind.clear();
        for (int i = 0; i < count; i++) {
            GTree[pos].mind.push_back(buf[i]);
        }
        pos++;
        delete[] buf;
    }
    fclose(fin);
}

void load_shortcuts() {
    FILE *fin = fopen(FILE_SHORTCUT, "rb");

    int *buf;
    int count;

    int kv1_size, kv2_size;
    fread(&kv1_size, sizeof(int), 1, fin);

    int i, j;

    for (int x = 0; x < kv1_size; ++x) {
        fread(&i, sizeof(int), 1, fin);
        fread(&kv2_size, sizeof(int), 1, fin);

        for (int y = 0; y < kv2_size; ++y) {
            fread(&j, sizeof(int), 1, fin);
            fread(&count, sizeof(int), 1, fin);

            buf = new int[count];
            fread(buf, sizeof(int), count, fin);
            shortcuts[i][j].clear();
            shortcuts[i][j].insert(shortcuts[i][j].end(), buf, buf + count);
            shortcuts[j][i].clear();
            shortcuts[j][i].emplace_back(-1);
            delete[] buf;
        }
    }
    fclose(fin);
}


// up_pos & current_pos(used for quickly locating parent & child nodes)
void build_up_and_down_pos() {
    unordered_map<int, int> pos_map;
    for (int i = 1; i < GTree.size(); i++) {
        GTree[i].current_pos.clear();
        GTree[i].up_pos.clear();

        // current_pos
        pos_map.clear();
        for (int j = 0; j < GTree[i].union_borders.size(); j++) {
            pos_map[GTree[i].union_borders[j]] = j;
        }
        for (int j = 0; j < GTree[i].borders.size(); j++) {
            GTree[i].current_pos.push_back(pos_map[GTree[i].borders[j]]);
        }
        // up_pos
        pos_map.clear();
        for (int j = 0; j < GTree[GTree[i].father].union_borders.size(); j++) {
            pos_map[GTree[GTree[i].father].union_borders[j]] = j;
        }
        for (int j = 0; j < GTree[i].borders.size(); j++) {
            GTree[i].up_pos.push_back(pos_map[GTree[i].borders[j]]);
        }
    }
}

void init() {
    load_graph();
    load_gtree();
    load_minds();
    load_shortcuts();
    build_up_and_down_pos();
}

void init_query() {
    for (auto &tn: GTree) {
        //tn.oclist.clear();
        tn.cache = vector<int>(tn.borders.size(), 0);
        tn.is_visited = false;
    }
}

vector<int> load_objects() {
    unordered_set<int> o;
    o.clear();

    FILE *fin = fopen(FILE_OBJECT, "r");
    int oid;
    while (fscanf(fin, "%d", &oid) == 1) {
        o.insert(oid);
    }
    fclose(fin);

    vector<int> res(o.begin(), o.end());

    return res;
}


inline int dijkstra_p2p(int s, int t) {
    unordered_map<int, int> result;
    result[s] = 0;

    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    pq.emplace(0, s);

    int min, minpos, adjnode, weight;
    TIME_TICK_START

    while (!pq.empty()) {
        min = pq.top().first;
        minpos = pq.top().second;

        if (minpos == t) {
            TIME_TICK_END
            return min;
        }

        pq.pop();

        for (int i = 0; i < Nodes[minpos].adjnodes.size(); i++) {

            adjnode = Nodes[minpos].adjnodes[i];
            weight = Nodes[minpos].adjweight[i];

            if (result.find(adjnode) == result.end() || result[adjnode] > min + weight) {
                result[adjnode] = min + weight;
                pq.emplace(min + weight, adjnode);
            }
        }
    }

    return -1;
}

inline vector<int> dijkstra_candidate(int s, unordered_set<int> &cands) {
    // init
    auto num_cands = cands.size();
    unordered_set<int> todo(cands.begin(), cands.end());
    unordered_map<int, int> result;
    result[s] = 0;

    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    pq.emplace(0, s);

    int min, minpos, adjnode, weight;
    while (!todo.empty() && !pq.empty()) {
        min = pq.top().first;
        minpos = pq.top().second;
        pq.pop();

        todo.erase(minpos);

        for (int i = 0; i < Nodes[minpos].adjnodes.size(); i++) {

            adjnode = Nodes[minpos].adjnodes[i];
            weight = Nodes[minpos].adjweight[i];

            if (result.find(adjnode) == result.end() || result[adjnode] > min + weight) {
                result[adjnode] = min + weight;
                pq.emplace(min + weight, adjnode);
            }
        }
    }

    // output
    vector<int> output;
    output.reserve(num_cands);
    for (const auto &iter : cands) {
        output.emplace_back(result[iter]);
    }

    // return
    return output;

}


inline vector<int> dijkstra_candidate(int s, vector<int> &cands) {
    // init
    auto num_cands = cands.size();
    unordered_set<int> todo(cands.begin(), cands.end());
    unordered_map<int, int> result;
    result[s] = 0;

    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    pq.emplace(0, s);

    int min, minpos, adjnode, weight;
    while (!todo.empty() && !pq.empty()) {
        min = pq.top().first;
        minpos = pq.top().second;
        pq.pop();

        todo.erase(minpos);

        for (int i = 0; i < Nodes[minpos].adjnodes.size(); i++) {

            adjnode = Nodes[minpos].adjnodes[i];
            weight = Nodes[minpos].adjweight[i];

            if (result.find(adjnode) == result.end() || result[adjnode] > min + weight) {
                result[adjnode] = min + weight;
                pq.emplace(min + weight, adjnode);
            }
        }
    }

    // output
    vector<int> output;
    output.reserve(num_cands);
    for (const auto &iter : cands) {
        output.emplace_back(result[iter]);
    }

    // return
    return output;

}

inline bool check_shortcut(int ns, int nt) {
    return shortcuts.find(ns) != shortcuts.end() && shortcuts[ns].find(nt) != shortcuts[ns].end();
}

inline int find_LCA_pos(int src, int dst) {
    for (int i = 1; i < Nodes[src].deep && i < Nodes[dst].deep; ++i) {
        if (Nodes[src].gtreepath[i] != Nodes[dst].gtreepath[i])
            return i - 1;
    }
    return 0;
}

inline int dist_query(int src, int dst) {
    int Ns = Nodes[src].inleaf;
    int Nt = Nodes[dst].inleaf;

    // Case D1: vertices src and dst are in the same leaf node
    if (Ns == Nt) {
        return dijkstra_p2p(src, dst);
    }

    int posa = Nodes[src].inleafpos;
    int posb = Nodes[dst].inleafpos;
    auto num_border_ns = GTree[Ns].borders.size();
    auto num_leafnode_ns = GTree[Ns].leafnodes.size();
    auto num_border_nt = GTree[Nt].borders.size();
    auto num_leafnode_nt = GTree[Nt].leafnodes.size();

    // Case D2: there is a shortcut between Ns and Nt
    if (check_shortcut(Ns, Nt)) {
        vector<int> sc = Ns < Nt ? shortcuts[Ns][Nt] : shortcuts[Nt][Ns];

        for (int i = 0; i < num_border_ns; ++i) {
            GTree[Ns].cache[i] = GTree[Ns].mind[i * num_leafnode_ns + posa];
        }

        for (int i = 0; i < num_border_nt; ++i) {
            GTree[Nt].cache[i] = GTree[Nt].mind[i * num_leafnode_nt + posb];
        }

        int min = INT_MAX, dist;

        if (Ns < Nt) {
            TIME_TICK_START
            for (int i = 0; i < num_border_ns; ++i) {
                for (int j = 0; j < num_border_nt; ++j) {
                    dist = GTree[Ns].cache[i] + GTree[Nt].cache[j] + sc[i * num_border_ns + j];
                    if (dist < min) {
                        min = dist;
                    }
                }
            }
            TIME_TICK_END
        } else {
            TIME_TICK_START
            for (int i = 0; i < num_border_ns; ++i) {
                for (int j = 0; j < num_border_nt; ++j) {
                    dist = GTree[Ns].cache[i] + GTree[Nt].cache[j] +  sc[j * num_border_nt + i];
                    if (dist < min) {
                        min = dist;
                    }
                }
            }
            TIME_TICK_END
        }
        return min;
    }

    // Case D3: there is no shortcut between Ns and Nt

    // Find LCA index in gtreepath
    int LCA_pos = find_LCA_pos(src, dst);

    // Step out of leaf node Ns
    for (int i = 0; i < num_border_ns; ++i) {
        GTree[Ns].cache[i] = GTree[Ns].mind[i * num_leafnode_ns + posa];
    }

    // Init some variables
    const auto &up_path = Nodes[src].gtreepath;
    const auto &down_path = Nodes[dst].gtreepath;
    int cn, tn, min, dist, posx, posy;
    unsigned long union_border_size;

    // Step out of nodes until meeting LCA
    // The cache of each node 'tn' stores the distance from vertex src to node tn's child then to tn
    TIME_TICK_START
    for (auto i = up_path.size() - 2; i >= LCA_pos + 1; --i) {
        tn = up_path[i];
        cn = up_path[i + 1];  // child node
        union_border_size = GTree[tn].union_borders.size();
        for (int j = 0; j < GTree[tn].borders.size(); j++) {
            min = INT_MAX;
            posx = GTree[tn].current_pos[j];
            for (int k = 0; k < GTree[cn].borders.size(); k++) {
                posy = GTree[cn].up_pos[k];
                dist = GTree[cn].cache[k] + GTree[tn].mind[posx * union_border_size + posy];
                if (dist < min) {
                    min = dist;
                }
            }
            GTree[tn].cache[j] = min;
        }
    }


    // Step across LCA (from one branch to another)
    // The cache of Nt's top ancestor node 'nt_top' stores the distance
    // from vertex src to Ns's top ancestor 'ns_top' node then to 'nt_top'

    int ns_top = up_path[LCA_pos + 1];
    int nt_top = down_path[LCA_pos + 1];
    int lca_node = up_path[LCA_pos];
    int ns_top_up_pos, nt_top_up_pos;
    union_border_size = GTree[lca_node].union_borders.size();
    for (int i = 0; i < GTree[nt_top].borders.size(); i++) {
        min = INT_MAX;
        nt_top_up_pos = GTree[nt_top].up_pos[i];
        for (int j = 0; j < GTree[ns_top].borders.size(); j++) {
            ns_top_up_pos = GTree[ns_top].up_pos[j];
            dist = GTree[ns_top].cache[i] + GTree[lca_node].mind[nt_top_up_pos * union_border_size + ns_top_up_pos];
            if (dist < min) {
                min = dist;
            }
        }
        GTree[nt_top].cache[i] = min;
    }


    // Step into nodes until meeting Nt
    // The cache of each node 'tn' stores the distance from vertex src to node tn's parent then to tn
    for (auto i = LCA_pos + 2; i < down_path.size(); ++i) {
        tn = down_path[i];
        cn = down_path[i - 1];   // parent node
        union_border_size = GTree[cn].union_borders.size();
        for (int j = 0; j < GTree[tn].borders.size(); j++) {
            min = INT_MAX;
            posx = GTree[tn].up_pos[j];
            for (int k = 0; k < GTree[cn].borders.size(); k++) {
                posy = GTree[cn].current_pos[k];
                dist = GTree[cn].cache[k] + GTree[cn].mind[posy * union_border_size + posx];
                if (dist < min) {
                    min = dist;
                }
            }
            // update
            GTree[tn].cache[j] = min;
        }
    }

    // Step into the leaf node Nt
    min = INT_MAX;
    for (int i = 0; i < num_border_nt; ++i) {
        dist = GTree[Nt].cache[i] + GTree[Nt].mind[i * num_leafnode_nt + posb];
        if (dist < min) {
            min = dist;
        }
    }
    TIME_TICK_END
    return min;
}

struct knn_item {
    int dist;
    int id;
    char type;  // 0: vertex, 1: leaf node, 2: non-leaf node

    knn_item(int _dist, int _id, bool _type) {
        dist = _dist;
        id = _id;
        type = _type;
    }

    bool operator<(const knn_item &rhs) const {
        return (dist > rhs.dist) || (dist == rhs.dist && type > rhs.type);
    }
};

inline int step_out(int &nc, priority_queue<knn_item> &q) {
    int oldnc = nc;
    nc = GTree[nc].father;
    int tmin = INT_MAX;

    if (nc != 0) {
        // Update cache for GTree[nc] and tmin
        // tmin = INT_MAX;
        int dist, min, posx, posy;

        int tn = nc;
        int cn = oldnc;  // child node
        auto union_border_size = GTree[tn].union_borders.size();
        for (int j = 0; j < GTree[tn].borders.size(); j++) {
            min = INT_MAX;
            posx = GTree[tn].current_pos[j];
            for (int k = 0; k < GTree[cn].borders.size(); k++) {
                posy = GTree[cn].up_pos[k];
                dist = GTree[cn].cache[k] + GTree[tn].mind[posx * union_border_size + posy];
                if (dist < min) {
                    min = dist;
                }
            }
            GTree[tn].cache[j] = min;
            if (min < tmin) tmin = min;
        }
    }

    // Build cache for every child in GTree[nc].oclist (step across node nc)
    for (const auto &nt_top : GTree[nc].oclist) {
        int ns_top = oldnc;
        if (ns_top == nt_top) continue;

        if (GTree[nt_top].oclist.empty()) continue;

        // distance from vertex src to node nt_top's nearest border
        int dist_nt_top = INT_MAX;

        int dist, min;
        int lca_node = nc;
        int ns_top_up_pos, nt_top_up_pos;
        auto union_border_size = GTree[lca_node].union_borders.size();
        for (int i = 0; i < GTree[nt_top].borders.size(); i++) {
            min = INT_MAX;
            nt_top_up_pos = GTree[nt_top].up_pos[i];
            for (int j = 0; j < GTree[ns_top].borders.size(); j++) {
                ns_top_up_pos = GTree[ns_top].up_pos[j];
                dist = GTree[ns_top].cache[i] +
                       GTree[lca_node].mind[nt_top_up_pos * union_border_size + ns_top_up_pos];
                if (dist < min) {
                    min = dist;
                }
            }
            GTree[nt_top].cache[i] = min;
            if (min < dist_nt_top) dist_nt_top = min;
        }

        q.emplace(dist_nt_top, nt_top, GTree[nt_top].isleaf ? 1 : 2);
    }

    return tmin;
}

inline void remove_leafnode_from_occurrence_list(int tn) {
    int father = GTree[tn].father;
    GTree[father].oclist.erase(tn);
    int oldfather;

    while (father > 0 && GTree[father].oclist.empty()) {
        oldfather = father;
        father = GTree[father].father;
        GTree[father].oclist.erase(oldfather);
    }
}

inline vector<int> knn_query(int src, int K) {
    for (int i = 0; i < Nodes[src].gtreepath.size() - 1; ++i) {
        GTree[i].oclist.erase(Nodes[src].gtreepath[i + 1]);
    }

    priority_queue<knn_item> q;
    int nc = Nodes[src].inleaf;

    if (!(GTree[nc].oclist.empty())) {
        auto dists = dijkstra_candidate(src, GTree[nc].oclist);
        auto iter1 = dists.begin();
        auto iter2 = GTree[nc].oclist.begin();
        for (; iter1 != dists.end() && iter2 != GTree[nc].oclist.end(); ++iter1, ++iter2) {
            q.emplace(*iter1, *iter2, 0);
        }
    }

    // Build cache for GTree[nc] and tmin;
    int tmin = INT_MAX;
    int src_inleaf_pos = Nodes[src].inleafpos;
    auto num_leafnodes_in_src_leaf = GTree[nc].leafnodes.size();
    for (int i = 0; i < GTree[nc].borders.size(); ++i) {
        GTree[nc].cache[i] = GTree[nc].mind[i * num_leafnodes_in_src_leaf + src_inleaf_pos];
        if (GTree[nc].cache[i] < tmin) {
            tmin = GTree[nc].cache[i];
        }
    }

    // Compute distances to nodes which have shortcuts with node GTree[src]
    unsigned long num_border_ns, num_border_nt;
    GTree[nc].is_visited = true;
//    if (shortcuts.find(nc) != shortcuts.end()) {
//        for (const auto &tn : shortcuts[nc]) {
//            int Ns = nc;
//            int Nt = tn.first;
//
//            if (GTree[Nt].oclist.empty()) continue;
//
//            vector<int> sc = Ns < Nt ? shortcuts[Ns][Nt] : shortcuts[Nt][Ns];
//
//            num_border_ns = GTree[Ns].borders.size();
//            num_border_nt = GTree[Nt].borders.size();
//
//            int tn_node_min = INT_MAX;
//            int min, dist;
//
//            for (int j = 0; j < num_border_nt; ++j) {
//                min = INT_MAX;
//                for (int i = 0; i < num_border_ns; ++i) {
//                    dist = GTree[Ns].cache[i] +
//                           (Ns < Nt ? sc[i * num_border_ns + j] : sc[j * num_border_nt + i]);
//                    if (dist < min) {
//                        min = dist;
//                    }
//                }
//                // Update cache of node Nt
//                GTree[Nt].cache[j] = min;
//                if (min < tn_node_min) tn_node_min = min;
//            }
//
//            GTree[Nt].is_visited = true;
//            remove_leafnode_from_occurrence_list(Nt);
//            q.emplace(tn_node_min, tn.first, GTree[Nt].isleaf ? 1 : 2);
//        }
//    }

    vector<int> result;
    result.reserve(K);

    while (result.size() < K && ((!q.empty()) || (nc != 0))) {
        if (q.empty()) {  // Step out
            tmin = step_out(nc, q);
            continue;
        }

        int min_dist = q.top().dist;

        if (min_dist > tmin) { // Step out
            tmin = step_out(nc, q);
        } else {
            int min_id = q.top().id;
            char min_type = q.top().type;

            q.pop();

            if (min_type == 0) {
                result.emplace_back(min_id);
            } else {
                if (GTree[min_id].isleaf) {

                    int min, dist, posb;
                    auto min_id_leafnodes_size = GTree[min_id].leafnodes.size();
                    for (const auto &vertex : GTree[min_id].oclist) {
                        posb = Nodes[vertex].inleafpos;
                        min = INT_MAX;
                        for (int j = 0; j < GTree[min_id].borders.size(); ++j) {
                            dist = GTree[min_id].cache[j] + GTree[min_id].mind[j * min_id_leafnodes_size + posb];
                            if (dist < min) {
                                min = dist;
                            }
                        }
                        q.emplace(min, vertex, 0);
                    }

                    // Compute distances to nodes which have shortcuts with node GTree[min_id]
                    if (shortcuts.find(min_id) != shortcuts.end()) {
                        for (const auto &tn : shortcuts[min_id]) {
                            int Ns = min_id;
                            int Nt = tn.first;

                            if (GTree[Nt].is_visited || GTree[Nt].oclist.empty()) continue;

                            vector<int> sc = Ns < Nt ? shortcuts[Ns][Nt] : shortcuts[Nt][Ns];

                            num_border_ns = GTree[Ns].borders.size();
                            num_border_nt = GTree[Nt].borders.size();

                            int tn_node_min = INT_MAX;

                            for (int j = 0; j < num_border_nt; ++j) {
                                min = INT_MAX;
                                for (int i = 0; i < num_border_ns; ++i) {
                                    dist = GTree[Ns].cache[i] +
                                           (Ns < Nt ? sc[i * num_border_ns + j] : sc[j * num_border_nt + i]);
                                    if (dist < min) {
                                        min = dist;
                                    }
                                }
                                // Update cache of node Nt
                                GTree[Nt].cache[j] = min;
                                if (min < tn_node_min) tn_node_min = min;
                            }

                            GTree[tn.first].is_visited = true;
                            remove_leafnode_from_occurrence_list(tn.first);
                            q.emplace(tn_node_min, tn.first, GTree[tn.first].isleaf ? 1 : 2);
                        }
                    }
                } else {
                    // Compute the distance from node min_id to its children
                    // Update the children's cache
                    int cn = min_id, posx, posy, dist, min, child_node_dist;
                    unsigned long union_border_size;
                    for (const auto tn : GTree[min_id].oclist) {
                        if (GTree[tn].isleaf) {
                            if (GTree[tn].is_visited) {
                                continue;
                            }
                            GTree[tn].is_visited = true;
                        }

                        if (GTree[tn].oclist.empty()) continue;

                        // tn = child
                        // cn = min_id;   // parent node
                        child_node_dist = INT_MAX;
                        union_border_size = GTree[cn].union_borders.size();
                        for (int j = 0; j < GTree[tn].borders.size(); j++) {
                            min = INT_MAX;
                            posx = GTree[tn].up_pos[j];
                            for (int k = 0; k < GTree[cn].borders.size(); k++) {
                                posy = GTree[cn].current_pos[k];
                                dist = GTree[cn].cache[k] + GTree[cn].mind[posy * union_border_size + posx];
                                if (dist < min) {
                                    min = dist;
                                }
                            }
                            // update
                            GTree[tn].cache[j] = min;
                            if (min < child_node_dist) child_node_dist = min;
                        }
                        q.emplace(child_node_dist, tn, GTree[tn].isleaf ? 1 : 2);
                    }
                }
            }
        }
    }

    return result;
}

struct range_item {
    int dist;
    int id;
    char type;  // 0: vertex, 1: leaf node, 2: non-leaf node

    range_item(int _dist, int _id, bool _type) {
        dist = _dist;
        id = _id;
        type = _type;
    }

    bool operator<(const range_item &rhs) const {
        return (dist > rhs.dist) || (dist == rhs.dist && type < rhs.type);
    }
};

inline int step_out_for_range_query(int &nc, priority_queue<range_item> &q, int eps) {
    int oldnc = nc;
    nc = GTree[nc].father;
    int tmin = INT_MAX;

    if (nc != 0) {
        // Update cache for GTree[nc] and tmin
        // tmin = INT_MAX;
        int dist, min, posx, posy;

        int tn = nc;
        int cn = oldnc;  // child node
        auto union_border_size = GTree[tn].union_borders.size();
        for (int j = 0; j < GTree[tn].borders.size(); j++) {
            min = INT_MAX;
            posx = GTree[tn].current_pos[j];
            for (int k = 0; k < GTree[cn].borders.size(); k++) {
                posy = GTree[cn].up_pos[k];
                dist = GTree[cn].cache[k] + GTree[tn].mind[posx * union_border_size + posy];
                if (dist < min) {
                    min = dist;
                }
            }
            GTree[tn].cache[j] = min;
            if (min < tmin) tmin = min;
        }
    }

    if (tmin > eps) return eps + 1;

    // Build cache for every child in GTree[nc].oclist (step across node nc)
    for (const auto &nt_top : GTree[nc].oclist) {
        int ns_top = oldnc;
        if (ns_top == nt_top) continue;

        if (GTree[nt_top].oclist.empty()) continue;

        // distance from vertex src to node nt_top's nearest border
        int dist_nt_top = INT_MAX;

        int dist, min;
        int lca_node = nc;
        int ns_top_up_pos, nt_top_up_pos;
        auto union_border_size = GTree[lca_node].union_borders.size();
        for (int i = 0; i < GTree[nt_top].borders.size(); i++) {
            min = INT_MAX;
            nt_top_up_pos = GTree[nt_top].up_pos[i];
            for (int j = 0; j < GTree[ns_top].borders.size(); j++) {
                ns_top_up_pos = GTree[ns_top].up_pos[j];
                dist = GTree[ns_top].cache[i] +
                       GTree[lca_node].mind[nt_top_up_pos * union_border_size + ns_top_up_pos];
                if (dist < min) {
                    min = dist;
                }
            }
            GTree[nt_top].cache[i] = min;
            if (min < dist_nt_top) dist_nt_top = min;
        }

        if (dist_nt_top <= eps) q.emplace(dist_nt_top, nt_top, GTree[nt_top].isleaf ? 1 : 2);
    }

    return tmin;
}

inline vector<int> range_query(int src, int eps) {
    for (int i = 0; i < Nodes[src].gtreepath.size() - 1; ++i) {
        GTree[i].oclist.erase(Nodes[src].gtreepath[i + 1]);
    }

    priority_queue<range_item> q;
    int nc = Nodes[src].inleaf;

    vector<int> result;

    if (!(GTree[nc].oclist.empty())) {
        auto dists = dijkstra_candidate(src, GTree[nc].oclist);
        auto iter1 = dists.begin();
        auto iter2 = GTree[nc].oclist.begin();
        for (; iter1 != dists.end() && iter2 != GTree[nc].oclist.end(); ++iter1, ++iter2) {
            if (*iter1 <= eps) result.emplace_back(*iter2);
        }
    }

    // Build cache for GTree[nc] and tmin;
    int tmin = INT_MAX;
    int src_inleaf_pos = Nodes[src].inleafpos;
    auto num_leafnodes_in_src_leaf = GTree[nc].leafnodes.size();
    for (int i = 0; i < GTree[nc].borders.size(); ++i) {
        GTree[nc].cache[i] = GTree[nc].mind[i * num_leafnodes_in_src_leaf + src_inleaf_pos];
        if (GTree[nc].cache[i] < tmin) {
            tmin = GTree[nc].cache[i];
        }
    }

    if (tmin > eps) return result;


    // Compute distances to nodes which have shortcuts with node GTree[src]
    unsigned long num_border_ns, num_border_nt;
    GTree[nc].is_visited = true;
//    if (shortcuts.find(nc) != shortcuts.end()) {
//        for (const auto &tn : shortcuts[nc]) {
//            int Ns = nc;
//            int Nt = tn.first;
//
//            if (GTree[Nt].oclist.empty()) continue;
//
//            vector<int> sc = Ns < Nt ? shortcuts[Ns][Nt] : shortcuts[Nt][Ns];
//
//            num_border_ns = GTree[Ns].borders.size();
//            num_border_nt = GTree[Nt].borders.size();
//
//            int tn_node_min = INT_MAX;
//            int min, dist;
//
//            for (int j = 0; j < num_border_nt; ++j) {
//                min = INT_MAX;
//                for (int i = 0; i < num_border_ns; ++i) {
//                    dist = GTree[Ns].cache[i] +
//                           (Ns < Nt ? sc[i * num_border_ns + j] : sc[j * num_border_nt + i]);
//                    if (dist < min) {
//                        min = dist;
//                    }
//                }
//                // Update cache of node Nt
//                GTree[Nt].cache[j] = min;
//                if (min < tn_node_min) tn_node_min = min;
//            }
//
//            GTree[Nt].is_visited = true;
//            remove_leafnode_from_occurrence_list(Nt);
//            if (tn_node_min < eps) q.emplace(tn_node_min, tn.first, GTree[Nt].isleaf ? 1 : 2);
//        }
//    }

    while (((!q.empty()) || (nc != 0))) {
        if (tmin > eps && nc != 0) return result;

        if (q.empty()) {  // Step out
            tmin = step_out_for_range_query(nc, q, eps);
            continue;
        }

        int min_dist = q.top().dist;

        if (min_dist > eps) return result;

        if (min_dist > tmin) { // Step out
            tmin = step_out_for_range_query(nc, q, eps);
        } else {
            int min_id = q.top().id;
            char min_type = q.top().type;

            q.pop();

            if (min_type == 0) {
                result.emplace_back(min_id);
            } else {
                if (GTree[min_id].isleaf) {

                    int min, dist, posb;
                    auto min_id_leafnodes_size = GTree[min_id].leafnodes.size();
                    for (const auto &vertex : GTree[min_id].oclist) {
                        posb = Nodes[vertex].inleafpos;
                        min = INT_MAX;
                        for (int j = 0; j < GTree[min_id].borders.size(); ++j) {
                            dist = GTree[min_id].cache[j] + GTree[min_id].mind[j * min_id_leafnodes_size + posb];
                            if (dist < min) {
                                min = dist;
                            }
                        }
                        if (min <= eps) result.emplace_back(vertex);
                    }

                    // Compute distances to nodes which have shortcuts with node GTree[min_id]
                    if (shortcuts.find(min_id) != shortcuts.end()) {
                        for (const auto &tn : shortcuts[min_id]) {
                            int Ns = min_id;
                            int Nt = tn.first;

                            if (GTree[Nt].is_visited || GTree[Nt].oclist.empty()) continue;

                            vector<int> sc = Ns < Nt ? shortcuts[Ns][Nt] : shortcuts[Nt][Ns];

                            num_border_ns = GTree[Ns].borders.size();
                            num_border_nt = GTree[Nt].borders.size();

                            int tn_node_min = INT_MAX;

                            for (int j = 0; j < num_border_nt; ++j) {
                                min = INT_MAX;
                                for (int i = 0; i < num_border_ns; ++i) {
                                    dist = GTree[Ns].cache[i] +
                                           (Ns < Nt ? sc[i * num_border_ns + j] : sc[j * num_border_nt + i]);
                                    if (dist < min) {
                                        min = dist;
                                    }
                                }
                                // Update cache of node Nt
                                GTree[Nt].cache[j] = min;
                                if (min < tn_node_min) tn_node_min = min;
                            }

                            GTree[tn.first].is_visited = true;
                            remove_leafnode_from_occurrence_list(tn.first);
                            if (tn_node_min <= eps) q.emplace(tn_node_min, tn.first, GTree[tn.first].isleaf ? 1 : 2);
                        }
                    }
                } else {
                    // Compute the distance from node min_id to its children
                    // Update the children's cache
                    int cn = min_id, posx, posy, dist, min, child_node_dist;
                    unsigned long union_border_size;
                    for (const auto tn : GTree[min_id].oclist) {
                        if (GTree[tn].isleaf) {
                            if (GTree[tn].is_visited) {
                                continue;
                            }
                            GTree[tn].is_visited = true;
                        }

                        if (GTree[tn].oclist.empty()) continue;

                        // tn = child
                        // cn = min_id;   // parent node
                        child_node_dist = INT_MAX;
                        union_border_size = GTree[cn].union_borders.size();
                        for (int j = 0; j < GTree[tn].borders.size(); j++) {
                            min = INT_MAX;
                            posx = GTree[tn].up_pos[j];
                            for (int k = 0; k < GTree[cn].borders.size(); k++) {
                                posy = GTree[cn].current_pos[k];
                                dist = GTree[cn].cache[k] + GTree[cn].mind[posy * union_border_size + posx];
                                if (dist < min) {
                                    min = dist;
                                }
                            }
                            // update
                            GTree[tn].cache[j] = min;
                            if (min < child_node_dist) child_node_dist = min;
                        }
                        if (child_node_dist <= eps) q.emplace(child_node_dist, tn, GTree[tn].isleaf ? 1 : 2);
                    }
                }
            }
        }
    }

    return result;
}


void dist_main(char *argv[]) {
    FILE_NODE = argv[2];
    FILE_EDGE = argv[3];

    FILE_NODES_GTREE_PATH = argv[4];
    FILE_GTREE = argv[5];
    FILE_ONTREE_MIND = argv[6];

    FILE_QUERY = argv[7];
    FILE_SHORTCUT = argv[8];

    // init
    init();
    int src, dst, res;

    long double query_time;

    FILE *fin;
    fin = fopen(FILE_QUERY, "r");

    long double total_time = 0.0;
    int count = 0;

    do {
        init_query();
        res = fscanf(fin, "%d %d", &src, &dst);

//        TIME_TICK_START
        dist_query(src, dst);
//        TIME_TICK_END
        query_time = te - ts;

        // printf("%.6Lf\n", query_time);
        total_time += query_time;
        count++;
    } while (res != EOF);
    fclose(fin);

    cout << FILE_SHORTCUT << "\t" << (total_time / count) << endl;
}

inline void build_occurrence_recursive(int tn) {
    for (const auto &child : GTree[tn].children) {
        if (GTree[child].isleaf) {
            if (GTree[child].oclist.empty()) continue;
        } else {
            build_occurrence_recursive(child);
        }
        GTree[tn].oclist.emplace(child);
    }
}

inline void build_occurrence_list() {
    for (const int i : query_objects) {
        GTree[Nodes[i].inleaf].oclist.emplace(i);
    }

    build_occurrence_recursive(0);
}

void knn_main(char *argv[]) {
    FILE_NODE = argv[2];
    FILE_EDGE = argv[3];

    FILE_NODES_GTREE_PATH = argv[4];
    FILE_GTREE = argv[5];
    FILE_ONTREE_MIND = argv[6];

    FILE_QUERY = argv[7];
    FILE_OBJECT = argv[8];
    FILE_SHORTCUT = argv[9];

    init();
    query_objects = load_objects();

    int src, K, res;

    long double query_time, pre_time = 0;

    vector<long double> pre_time_vector;
    for (int i = 1; i <= 11; ++i) {
        TIME_TICK_START
        build_occurrence_list();
        TIME_TICK_END
        pre_time_vector.emplace_back(te - ts);
    }
    std::sort(pre_time_vector.begin(), pre_time_vector.end());
    pre_time = pre_time_vector[5];

    FILE *fin;
    fin = fopen(FILE_QUERY, "r");

    long double total_time = 0.0;
    int count = 0;

    do {
        init_query();
        res = fscanf(fin, "%d %d", &src, &K);

        TIME_TICK_START
        knn_query(src, K);
        TIME_TICK_END
        query_time = te - ts;

//        printf("%.6Lf %.6Lf %.6Lf\n", pre_time, query_time, pre_time + query_time);

        total_time += query_time;
        ++count;

    } while (res != EOF);
    fclose(fin);

    cout << FILE_SHORTCUT << "\t" << (total_time / count) << endl;
}


void range_main(char *argv[]) {
    FILE_NODE = argv[2];
    FILE_EDGE = argv[3];

    FILE_NODES_GTREE_PATH = argv[4];
    FILE_GTREE = argv[5];
    FILE_ONTREE_MIND = argv[6];

    FILE_QUERY = argv[7];
    FILE_OBJECT = argv[8];
    FILE_SHORTCUT = argv[9];

    init();
    query_objects = load_objects();

    int src, eps, res;

    long double query_time, pre_time;

    vector<long double> pre_time_vector;
    for (int i = 1; i <= 11; ++i) {
        TIME_TICK_START
        build_occurrence_list();
        TIME_TICK_END
        pre_time_vector.emplace_back(te - ts);
    }
    std::sort(pre_time_vector.begin(), pre_time_vector.end());
    pre_time = pre_time_vector[5];

    FILE *fin;
    fin = fopen(FILE_QUERY, "r");

    long double total_time = 0.0;
    int count = 0;

    do {
        init_query();
        res = fscanf(fin, "%d %d", &src, &eps);

        TIME_TICK_START
        range_query(src, eps);
        TIME_TICK_END
        query_time = te - ts;

//        printf("%.6Lf %.6Lf %.6Lf\n", pre_time, query_time, pre_time + query_time);

        total_time += query_time;
        ++count;

    } while (res != EOF);
    fclose(fin);

    cout << FILE_SHORTCUT << "\t" << (total_time / count) << endl;
}

int main(int argc, char *argv[]) {

    for (int i = 0; i < argc; ++i) {
        cerr << argv[i] << endl;
    }

    if (strcmp(argv[1], "knn") == 0) {
        knn_main(argv);
    } else if (strcmp(argv[1], "dist") == 0) {
        dist_main(argv);
    } else if (strcmp(argv[1], "range") == 0) {
        range_main(argv);
    }

    return 0;
}
