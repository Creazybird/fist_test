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

const char *FILE_SHORTCUT = "";
unsigned long SHORTCUT_THRESHOLD = 0;


// MACRO for timing
struct timeval tv;
long double ts, te;
#define TIME_TICK_START gettimeofday( &tv, NULL ); ts = (long double) tv.tv_sec * 1000.0 + (long double) tv.tv_usec / 1000.0;
#define TIME_TICK_END gettimeofday( &tv, NULL ); te = (long double) tv.tv_sec * 1000.0 + (long double) tv.tv_usec / 1000.0;
// ----------

struct Node {
    bool isborder;
    vector<int> adjnodes;
    vector<int> adjweight;
    vector<int> gtreepath; // this is used to do sub-graph locating
};   // Vertex

struct TreeNode {
    vector<int> borders;
    vector<int> children;
    bool isleaf;
    vector<int> leafnodes;
    int father;
// ----- min dis -----
    vector<int> union_borders; // for non leaf node
    vector<int> mind; // min dis, row by row of union_borders
// ----- for locating distance values in matrices
    vector<int> up_pos;
    vector<int> current_pos;
// ----- for caching distances
    vector<vector<int>> cache;
    bool is_cached;
};

int noe; // number of edges
vector<Node> Nodes;
vector<TreeNode> GTree;

vector<int> leaf_nodes;

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
        }
        // father
        fread(&father, sizeof(int), 1, fin);
        tn.father = father;

        GTree.push_back(tn);

        if (tn.isleaf) {
            leaf_nodes.push_back(node_count);
        }

        int i = node_count;

        GTree[i].is_cached = false;
        int j = i, father;
        while ((father = GTree[j].father) > 0) j = father;
        GTree[i].cache.resize(GTree[j].borders.size(), vector<int>(GTree[i].borders.size(), 0));

        ++node_count;

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

struct leaf_node_pair {
    int x, y;
    double v;

    leaf_node_pair(int _x, int _y, double _v) {
        x = _x;
        y = _y;
        v = _v;
    }

    bool operator<(const leaf_node_pair &rhs) const {
        return v < rhs.v;   // top item is largest
    }
};

inline double compute_value_of_leaf_node_pair(int i, int j) {
    double w =
            double(GTree[i].leafnodes.size()) * GTree[j].leafnodes.size() / (GTree[i].borders.size() * GTree[j].borders.size());
    double v = GTree[i].borders.size() + GTree[j].borders.size();

    int father;

    while ((father = GTree[i].father) != 0) {
        v += GTree[father].borders.size() * GTree[i].borders.size();
        i = father;
    }

    while ((father = GTree[j].father) != 0) {
        v += GTree[father].borders.size() * GTree[j].borders.size();
        j = father;
    }

    v += GTree[i].borders.size() * GTree[j].borders.size();

    return v * w;
}

bool check_leaf_adjacent(TreeNode &li, TreeNode &lj) {

    for (const auto &bi : li.borders) {
        for (const auto &bj : lj.borders) {
            if (find(Nodes[bi].adjnodes.begin(), Nodes[bi].adjnodes.end(), bj) != Nodes[bi].adjnodes.end()) {
                return true;
            }
        }
    }
    return false;
}

inline void build_shortcuts_for_nodes(int x, int y) {
    const auto &path_x = Nodes[GTree[x].leafnodes[0]].gtreepath;
    const auto &path_y = Nodes[GTree[y].leafnodes[0]].gtreepath;

    if (!GTree[x].is_cached) {
        const auto &path = path_x;
        int tn, cn, min, dist, posa, posb, posz, poszz;
        unsigned long union_border_size;

        for (int z = 0; z < GTree[path[1]].borders.size(); ++z) {

            if (!GTree[path[1]].is_cached) {
                posz = GTree[path[1]].up_pos[z];
                for (int zz = 0; zz < GTree[path[1]].borders.size(); ++zz) {
                    poszz = GTree[path[1]].up_pos[zz];
                    GTree[path[1]].cache[z][zz] = GTree[0].mind[posz * GTree[0].union_borders.size() + poszz];
                }
            }

            for (int i = 1; i < path.size() - 1; ++i) {
                tn = path[i];
                cn = path[i + 1];

                if (!GTree[cn].is_cached) {
                    for (int j = 0; j < GTree[cn].borders.size(); ++j) {
                        union_border_size = GTree[tn].union_borders.size();
                        min = INT_MAX;
                        posa = GTree[cn].up_pos[j];
                        for (int k = 0; k < GTree[tn].borders.size(); ++k) {
                            posb = GTree[tn].current_pos[k];
                            dist = GTree[tn].cache[z][k] + GTree[tn].mind[posb * union_border_size + posa];
                            if (dist < min) min = dist;
                        }
                        GTree[cn].cache[z][j] = min;
                    }
                }
            }
        }

        for (int i = 1; i < path.size(); ++i) {
            GTree[path[i]].is_cached = true;
        }
    }

    if (!GTree[y].is_cached) {
        const auto &path = path_y;
        int tn, cn, min, dist, posa, posb, posz, poszz;
        unsigned long union_border_size;

        for (int z = 0; z < GTree[path[1]].borders.size(); ++z) {

            if (!GTree[path[1]].is_cached) {
                posz = GTree[path[1]].up_pos[z];
                for (int zz = 0; zz < GTree[path[1]].borders.size(); ++zz) {
                    poszz = GTree[path[1]].up_pos[zz];
                    GTree[path[1]].cache[z][zz] = GTree[0].mind[posz * GTree[0].union_borders.size() + poszz];
                }
            }

            for (int i = 1; i < path.size() - 1; ++i) {
                tn = path[i];
                cn = path[i + 1];

                if (!GTree[cn].is_cached) {
                    for (int j = 0; j < GTree[cn].borders.size(); ++j) {
                        union_border_size = GTree[tn].union_borders.size();
                        min = INT_MAX;
                        posa = GTree[cn].up_pos[j];
                        for (int k = 0; k < GTree[tn].borders.size(); ++k) {
                            posb = GTree[tn].current_pos[k];
                            dist = GTree[tn].cache[z][k] + GTree[tn].mind[posb * union_border_size + posa];
                            if (dist < min) min = dist;
                        }
                        GTree[cn].cache[z][j] = min;
                    }
                }
            }
        }

        for (int i = 1; i < path.size(); ++i) {
            GTree[path[i]].is_cached = true;
        }
    }


    int ns_top = path_x[1];
    int nt_top = path_y[1];
    int ns_top_up_pos, nt_top_up_pos;
    auto union_border_size = GTree[0].union_borders.size();
    int min, dist;
    vector<vector<int>> mid(GTree[ns_top].borders.size(), vector<int>(GTree[nt_top].borders.size(), 0));

    for (int i = 0; i < GTree[ns_top].borders.size(); i++) {
        ns_top_up_pos = GTree[ns_top].up_pos[i];
        for (int j = 0; j < GTree[nt_top].borders.size(); j++) {
            nt_top_up_pos = GTree[nt_top].up_pos[j];
            mid[i][j] = GTree[0].mind[ns_top_up_pos * union_border_size + nt_top_up_pos];
        }
    }

    for (int i = 0; i < GTree[x].borders.size(); ++i) {
        for (int j = 0; j < GTree[y].borders.size(); ++j) {
            min = INT_MAX;
            for (int z = 0; z < GTree[ns_top].borders.size(); ++z) {
                for (int zz = 0; zz < GTree[nt_top].borders.size(); ++zz) {
                    dist = GTree[x].cache[z][i] + GTree[y].cache[zz][j] + mid[z][zz];
                    if (dist < min) {
                        min = dist;
                    }
                }
            }
            shortcuts[x][y].emplace_back(min);
        }
    }

}

inline void build_shortcuts() {

    priority_queue<leaf_node_pair> leaf_node_pairs;

    const auto n = double(SHORTCUT_THRESHOLD);
    double new_value;

    long count = 0;
    double value_sum = 0.0;

//    for (int i = 0; i < leaf_nodes.size() - 1; ++i) {
//        for (int j = i + 1; j < leaf_nodes.size(); ++j) {
//            if (GTree[leaf_nodes[i]].father != GTree[leaf_nodes[j]].father &&
//                check_leaf_adjacent(GTree[leaf_nodes[i]], GTree[leaf_nodes[j]])) {
//                new_value = compute_value_of_leaf_node_pair(leaf_nodes[i], leaf_nodes[j]);
//                if (leaf_node_pairs.size() < n) {
//                    leaf_node_pairs.emplace(leaf_nodes[i], leaf_nodes[j], new_value);
//                } else if (leaf_node_pairs.top().v < new_value) {
//                    leaf_node_pairs.pop();
//                    leaf_node_pairs.emplace(leaf_nodes[i], leaf_nodes[j], new_value);
//                }
//            }
//            printf("+%ld\n", count++);
//        }
//    }

    for (int i = 0; i < leaf_nodes.size() - 1; ++i) {
        for (int j = i + 1; j < leaf_nodes.size(); ++j) {
            if (GTree[leaf_nodes[i]].father != GTree[leaf_nodes[j]].father &&
                check_leaf_adjacent(GTree[leaf_nodes[i]], GTree[leaf_nodes[j]])) {
                new_value = compute_value_of_leaf_node_pair(leaf_nodes[i], leaf_nodes[j]);
                if (value_sum < n) {
                    leaf_node_pairs.emplace(leaf_nodes[i], leaf_nodes[j], new_value);
                    value_sum += (GTree[i].borders.size() * GTree[j].borders.size());
                } else if (leaf_node_pairs.top().v < new_value) {
                    leaf_node_pairs.pop();
                    leaf_node_pairs.emplace(leaf_nodes[i], leaf_nodes[j], new_value);
                }
            }
            count++;
            if (count % 1000000 == 0) printf("+%ld\n", count);
        }
    }

    count = leaf_node_pairs.size();

    cout << "count = " << count << endl;

    int idx_i, idx_j;
    while (!leaf_node_pairs.empty()) {
        idx_i = leaf_node_pairs.top().x;
        idx_j = leaf_node_pairs.top().y;

//        cout << "x = " << idx_i << "; y = " << idx_j << endl;
        build_shortcuts_for_nodes(idx_i, idx_j);

        leaf_node_pairs.pop();
        --count;
        if (count % 10 == 0) printf("-%ld\n", count);
    }
}

void save_shortcuts(string FS) {
    cout << "Begin saving shortcuts to file: " << FS << endl;
    FILE *fout = fopen(FS.c_str(), "wb");

    int *buf;
    unsigned long count;

    int kv1_size = shortcuts.size();
    fwrite(&kv1_size, sizeof(int), 1, fout);

    for (auto &kv1 : shortcuts) {
        int i = kv1.first;
        fwrite(&i, sizeof(int), 1, fout);

        int kv2_size = kv1.second.size();
        fwrite(&kv2_size, sizeof(int), 1, fout);

        for (auto &kv2 : kv1.second) {
            auto j = kv2.first;
            fwrite(&j, sizeof(int), 1, fout);

            count = kv2.second.size();
            fwrite(&count, sizeof(int), 1, fout);

            buf = new int[count];
            copy(kv2.second.begin(), kv2.second.end(), buf);
            fwrite(buf, sizeof(int), count, fout);

            delete[] buf;
        }
    }

    fclose(fout);
}

int main(int argc, char *argv[]) {

    for (int i = 0; i < argc; ++i) {
        printf("%s\n", argv[i]);
    }

    FILE_NODE = argv[1];
    FILE_EDGE = argv[2];

    FILE_NODES_GTREE_PATH = argv[3];
    FILE_GTREE = argv[4];
    FILE_ONTREE_MIND = argv[5];

    load_graph();
    load_gtree();
    load_minds();
    build_up_and_down_pos();

    vector<unsigned long> eta_list = {100000, 500000, 1000000, 1500000, 3000000};

    for (auto eta : eta_list) {
        SHORTCUT_THRESHOLD = eta;

        build_shortcuts();
        save_shortcuts(string(argv[6]) + "-" + std::to_string(eta) + ".sc");
    }

    return 0;
}
