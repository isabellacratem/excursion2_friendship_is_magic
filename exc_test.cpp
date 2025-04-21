#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <limits>
#include <algorithm>

using namespace std;

enum class NodeType {
    AND, OR, NOT, INPUT, OUTPUT, NAND2, NOR2, AOI21, AOI22
};

struct Node {
    string name;
    NodeType type;
    vector<string> inputs;
    int cost;
    bool visited;
    Node() : type(NodeType::INPUT), cost(-1), visited(false) {}
};

static const int NOT_COST = 2;
static const int NAND2_COST = 3;
static const int AND2_COST = 4;
static const int NOR2_COST = 6;
static const int OR2_COST = 4;
static const int AOI21_COST = 7;
static const int AOI22_COST = 7;

static string trim(const string &s) {
    size_t f = s.find_first_not_of(" \t\r\n");
    if (f == string::npos) return "";
    size_t l = s.find_last_not_of(" \t\r\n");
    return s.substr(f, l - f + 1);
}

unordered_map<string, Node> nodes;
string outputNode;

bool readNetlist(const string &fname) {
    ifstream f(fname);
    if (!f.is_open()) return false;
    string line;
    while (getline(f, line)) {
        line = trim(line);
        if (line.empty() || line.rfind("Test", 0) == 0 || line.rfind("Script", 0) == 0)
            continue;
        istringstream iss(line);
        string nm, op;
        iss >> nm >> op;
        if (op == "INPUT") {
            Node &n = nodes[nm];
            n.name = nm;
            n.type = NodeType::INPUT;
            n.cost = 0;
        } else if (op == "OUTPUT") {
            outputNode = nm;
        } else if (op == "=") {
            string gt;
            iss >> gt;
            Node &n = nodes[nm];
            n.name = nm;
            if (gt == "NOT") {
                n.type = NodeType::NOT;
                string a; iss >> a;
                n.inputs = {a};
            } else if (gt == "AND") {
                n.type = NodeType::AND;
                string a, b; iss >> a >> b;
                n.inputs = {a, b};
            } else if (gt == "OR") {
                n.type = NodeType::OR;
                string a, b; iss >> a >> b;
                n.inputs = {a, b};
            } else if (gt == "NAND2") {
                n.type = NodeType::NAND2;
                string a, b; iss >> a >> b;
                n.inputs = {a, b};
            } else if (gt == "NOR2") {
                n.type = NodeType::NOR2;
                string a, b; iss >> a >> b;
                n.inputs = {a, b};
            } else if (gt == "AOI21") {
                n.type = NodeType::AOI21;
                string a, b, c; iss >> a >> b >> c;
                n.inputs = {a, b, c};
            } else if (gt == "AOI22") {
                n.type = NodeType::AOI22;
                string a, b, c, d; iss >> a >> b >> c >> d;
                n.inputs = {a, b, c, d};
            } else if (iss.peek() == EOF) {
                n.type = NodeType::OUTPUT;
                n.inputs = {gt};
            }
        }
    }
    return !outputNode.empty();
}

int eval(const string &nm) {
    Node &n = nodes[nm];
    if (n.visited && n.cost >= 0) return n.cost;
    n.visited = true;

    if (n.type == NodeType::INPUT) return n.cost = 0;
    if (n.type == NodeType::OUTPUT) return n.cost = eval(n.inputs[0]);

    if (n.type == NodeType::NOT) {
        Node &child = nodes[n.inputs[0]];
        if (child.type == NodeType::AND && child.inputs.size() == 2) {
            int c1 = eval(child.inputs[0]);
            int c2 = eval(child.inputs[1]);
            return n.cost = c1 + c2 + NAND2_COST;
        }
        if (child.type == NodeType::OR && child.inputs.size() == 2) {
            int c1 = eval(child.inputs[0]);
            int c2 = eval(child.inputs[1]);
            return n.cost = c1 + c2 + NOR2_COST;
        }
        int c = eval(n.inputs[0]);
        return n.cost = min(NOT_COST + c, NAND2_COST + c);
    }

    int sum = 0;
    for (const string &in : n.inputs) {
        int c = eval(in);
        if (c < 0) return -1;
        sum += c;
    }

    if (n.type == NodeType::AND && n.inputs.size() == 2) {
        Node &a = nodes[n.inputs[0]];
        Node &b = nodes[n.inputs[1]];
        if (a.type == NodeType::NOT && b.type == NodeType::NOT) {
            int c1 = eval(a.inputs[0]);
            int c2 = eval(b.inputs[0]);
            return n.cost = c1 + c2 + 2 * NOT_COST + AND2_COST;
        }
    }

    int best = numeric_limits<int>::max();
    switch (n.type) {
        case NodeType::NOT:
            best = min(NOT_COST + sum, NAND2_COST + sum);
            break;
        case NodeType::AND:
            best = min(AND2_COST + sum, NAND2_COST + NOT_COST + sum);
            break;
        case NodeType::OR:
            best = min({OR2_COST + sum, NOR2_COST + NOT_COST + sum, 2 * NOT_COST + NAND2_COST + sum});
            break;
        case NodeType::NAND2:
            best = NAND2_COST + sum;
            break;
        case NodeType::NOR2:
            best = min(NOR2_COST + sum, 3 * NOT_COST + NAND2_COST + sum);
            break;
        case NodeType::AOI21:
            best = AOI21_COST + sum;
            break;
        case NodeType::AOI22:
            best = AOI22_COST + sum;
            break;
        default:
            return -1;
    }
    return n.cost = best;
}

int main() {
    if (!readNetlist("input8.txt")) return 1;
    for (auto &p : nodes) {
        p.second.visited = false;
        p.second.cost = -1;
    }
    int c = eval(outputNode);
    if (c < 0) return 1;
    ofstream out("output.txt"); out << c;
    cout << "Minimal cost: " << c << endl;
    return 0;
}
