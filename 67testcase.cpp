#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <limits>

// Node types in the circuit
enum class NodeType {
    AND,
    OR,
    NOT,
    INPUT,
    OUTPUT,
    NAND2,
    NOR2,
    AOI21,
    AOI22
};

// Structure to represent a node
struct Node {
    std::string name;
    NodeType type;
    std::vector<std::string> inputs;
    int cost;
    bool visited;
    Node() : type(NodeType::INPUT), cost(-1), visited(false) {}
};

// Gate costs
static const int NOT_COST   = 2;
static const int NAND2_COST = 3;
static const int AND2_COST  = 4;
static const int NOR2_COST  = 6;
static const int OR2_COST   = 4;
static const int AOI21_COST = 7;
static const int AOI22_COST = 7;

// Utility to trim whitespace
static std::string trim(const std::string &s) {
    size_t f = s.find_first_not_of(" \t\r\n");
    if (f == std::string::npos) return "";
    size_t l = s.find_last_not_of(" \t\r\n");
    return s.substr(f, l - f + 1);
}

class TechnologyMapper {
    std::unordered_map<std::string, Node> nodes;
    std::string outputNode;

public:
    bool readNetlist(const std::string &fname) {
        std::ifstream f(fname);
        if (!f.is_open()) return false;
        std::string line;
        while (std::getline(f, line)) {
            line = trim(line);
            if (line.empty() || line.rfind("Test", 0) == 0 || line.rfind("Script", 0) == 0)
                continue;
            std::istringstream iss(line);
            std::string nm, op;
            iss >> nm >> op;
            if (op == "INPUT") {
                Node &n = nodes[nm];
                n.name = nm;
                n.type = NodeType::INPUT;
                n.cost = 0;
            } else if (op == "OUTPUT") {
                outputNode = nm;
            } else if (op == "=") {
                std::string gt;
                iss >> gt;
                Node &n = nodes[nm];
                n.name = nm;
                if (gt == "NOT") {
                    n.type = NodeType::NOT;
                    std::string a; iss >> a;
                    n.inputs = {a};
                } else if (gt == "AND") {
                    n.type = NodeType::AND;
                    std::string a,b; iss >> a >> b;
                    n.inputs = {a,b};
                } else if (gt == "OR") {
                    n.type = NodeType::OR;
                    std::string a,b; iss >> a >> b;
                    n.inputs = {a,b};
                } else if (gt == "NAND2") {
                    n.type = NodeType::NAND2;
                    std::string a,b; iss >> a >> b;
                    n.inputs = {a,b};
                } else if (gt == "NOR2") {
                    n.type = NodeType::NOR2;
                    std::string a,b; iss >> a >> b;
                    n.inputs = {a,b};
                } else if (gt == "AOI21") {
                    n.type = NodeType::AOI21;
                    std::string a,b,c; iss >> a >> b >> c;
                    n.inputs = {a,b,c};
                } else if (gt == "AOI22") {
                    n.type = NodeType::AOI22;
                    std::string a,b,c,d; iss >> a >> b >> c >> d;
                    n.inputs = {a,b,c,d};
                } else if (iss.peek() == EOF) {
                    n.type = NodeType::OUTPUT;
                    n.inputs = {gt};
                }
            }
        }
        return !outputNode.empty();
    }

    int calculateMinimalCost() {
        for (auto &p : nodes) {
            p.second.visited = false;
            p.second.cost = -1;
        }
        return eval(outputNode);
    }

private:
    int eval(const std::string &nm) {
        Node &n = nodes[nm];
        // --- NOT-node patterns ---
        if (n.type == NodeType::NOT) {
            const std::string &c = n.inputs[0];
            // double-negation: NOT(NOT(x)) -> x
            if (nodes[c].type == NodeType::NOT)
                return eval(nodes[c].inputs[0]);
            // NOT(OR(a,b)) -> NOR2(a,b)
            if (nodes[c].type == NodeType::OR) {
                auto &in = nodes[c].inputs;
                int c0 = eval(in[0]);
                int c1 = eval(in[1]);
                return (c0<0||c1<0) ? -1 : c0 + c1 + NOR2_COST;
            }
            // NOT(OR(AND,...)) -> AOI21/AOI22
            if (nodes[c].type == NodeType::OR) {
                auto &in = nodes[c].inputs;
                bool a0 = nodes[in[0]].type==NodeType::AND;
                bool a1 = nodes[in[1]].type==NodeType::AND;
                // AOI21
                if (a0 && !a1) {
                    auto &v = nodes[in[0]].inputs;
                    int x=eval(v[0]), y=eval(v[1]), z=eval(in[1]);
                    return (x<0||y<0||z<0)?-1 : x+y+z+AOI21_COST;
                }
                if (!a0 && a1) {
                    auto &v = nodes[in[1]].inputs;
                    int x=eval(v[0]), y=eval(v[1]), z=eval(in[0]);
                    return (x<0||y<0||z<0)?-1 : x+y+z+AOI21_COST;
                }
                // AOI22
                if (a0 && a1) {
                    auto &v0=nodes[in[0]].inputs, &v1=nodes[in[1]].inputs;
                    int x=eval(v0[0]), y=eval(v0[1]), u=eval(v1[0]), v2=eval(v1[1]);
                    return (x<0||y<0||u<0||v2<0)?-1 : x+y+u+v2+AOI22_COST;
                }
            }
        }
        // --- AND-node patterns ---
        if (n.type == NodeType::AND) {
            auto &in = n.inputs;
            // Pattern: AND(AND(a,b), NOT(OR(c,d))) -> NOR2(NAND2(a,b), OR(c,d))
            if (nodes[in[0]].type == NodeType::AND &&
                nodes[in[1]].type == NodeType::NOT &&
                nodes[nodes[in[1]].inputs[0]].type == NodeType::OR) {
                auto &ab = nodes[in[0]].inputs;
                auto &cd = nodes[nodes[in[1]].inputs[0]].inputs;
                int ca = eval(ab[0]); if (ca<0) return -1;
                int cb = eval(ab[1]); if (cb<0) return -1;
                int cc = eval(cd[0]); if (cc<0) return -1;
                int cdv= eval(cd[1]); if (cdv<0) return -1;
                int costNand = ca + cb + NAND2_COST;
                int costOr   = cc + cdv + OR2_COST;
                return costNand + costOr + NOR2_COST;
            }
            // Pattern: AND(NOT(OR(c,d)), AND(a,b)) -> NOR2(OR(c,d), NAND2(a,b))
            if (nodes[in[1]].type == NodeType::AND &&
                nodes[in[0]].type == NodeType::NOT &&
                nodes[nodes[in[0]].inputs[0]].type == NodeType::OR) {
                auto &ab = nodes[in[1]].inputs;
                auto &cd = nodes[nodes[in[0]].inputs[0]].inputs;
                int ca = eval(ab[0]); if (ca<0) return -1;
                int cb = eval(ab[1]); if (cb<0) return -1;
                int cc = eval(cd[0]); if (cc<0) return -1;
                int cdv= eval(cd[1]); if (cdv<0) return -1;
                int costNand = ca + cb + NAND2_COST;
                int costOr   = cc + cdv + OR2_COST;
                return costNand + costOr + NOR2_COST;
            }
        }
        // memo
        if (n.visited && n.cost>=0) return n.cost;
        n.visited = true;
        // base
        if (n.type==NodeType::INPUT)  return n.cost=0;
        if (n.type==NodeType::OUTPUT) return n.cost=eval(n.inputs[0]);
        // generic sum
        int sum=0;
        for(auto &ch:n.inputs){int c=eval(ch); if(c<0) return -1; sum+=c;}
        int best=std::numeric_limits<int>::max();
        switch(n.type){
            case NodeType::NOT:
                best = std::min(NOT_COST+sum, NAND2_COST+sum);
                break;
            case NodeType::AND:
                best = std::min(AND2_COST+sum, NAND2_COST+NOT_COST+sum);
                break;
            case NodeType::OR:
                best = std::min({OR2_COST+sum, NOR2_COST+NOT_COST+sum, 2*NOT_COST+NAND2_COST+sum});
                break;
            case NodeType::NAND2:
                best = NAND2_COST+sum;
                break;
            case NodeType::NOR2:
                best = std::min(NOR2_COST+sum, 3*NOT_COST+NAND2_COST+sum);
                break;
            case NodeType::AOI21:
                best = AOI21_COST+sum;
                break;
            case NodeType::AOI22:
                best = AOI22_COST+sum;
                break;
            default:
                return -1;
        }
        return n.cost = best;
    }
};

int main(){
    TechnologyMapper tm;
    if(!tm.readNetlist("input8.txt")) return 1;
    int c = tm.calculateMinimalCost();
    if(c<0) return 1;
    std::ofstream out("output.txt"); out<<c;
    std::cout<<"Minimal cost: "<<c<<std::endl;
    return 0;
}
