// nand_not_mapper.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>

using namespace std;

enum class NodeType { AND, OR, NOT, INPUT, OUTPUT };

struct Node {
    string name;
    NodeType type;
    vector<string> inputs;
    int cost = -1;
    bool visited = false;

    Node() = default;
    Node(string n, NodeType t, vector<string> in) : name(n), type(t), inputs(in) {}
};

unordered_map<string, Node> nodes;
string output_name;

// Technology library costs
const int NOT_COST   = 2;
const int NAND2_COST = 3;
const int AND2_COST  = 4;
const int NOR2_COST  = 6;
const int OR2_COST   = 4;
const int AOI21_COST = 7;
const int AOI22_COST = 7;

int compute_cost(const string& name) {
    if (nodes.find(name) == nodes.end()) {
        cerr << "Error: Undefined node '" << name << "' used as input." << endl;
        exit(1);
    }

    Node& node = nodes[name];
    if (node.visited) return node.cost;
    node.visited = true;

    int total = 0;
    if (node.type == NodeType::INPUT) {
        total = 0;
    } else if (node.type == NodeType::NOT) {
        int a = compute_cost(node.inputs[0]);
        total = a + NOT_COST;

        // Detect NOR2 pattern: NOT(OR a b)
        if (nodes[node.inputs[0]].type == NodeType::OR) {
            const auto& or_node = nodes[node.inputs[0]];
            int a_cost = compute_cost(or_node.inputs[0]);
            int b_cost = compute_cost(or_node.inputs[1]);
            int folded_nor2 = a_cost + b_cost + NOR2_COST;
            total = min(total, folded_nor2);
        }

        // Detect AOI21 pattern: NOT(OR a AND b c)
        if (nodes[node.inputs[0]].type == NodeType::OR) {
            const auto& or_node = nodes[node.inputs[0]];
            string a = or_node.inputs[0];
            string bc = or_node.inputs[1];
            if (nodes[bc].type == NodeType::AND) {
                int ca = compute_cost(a);
                int cb = compute_cost(nodes[bc].inputs[0]);
                int cc = compute_cost(nodes[bc].inputs[1]);
                int aoi21 = ca + cb + cc + AOI21_COST;
                total = min(total, aoi21);
            }
        }

        // Detect AOI22: NOT(OR AND a b AND c d)
        if (nodes[node.inputs[0]].type == NodeType::OR) {
            const auto& or_node = nodes[node.inputs[0]];
            string ab = or_node.inputs[0];
            string cd = or_node.inputs[1];
            if (nodes[ab].type == NodeType::AND && nodes[cd].type == NodeType::AND) {
                int ca = compute_cost(nodes[ab].inputs[0]);
                int cb = compute_cost(nodes[ab].inputs[1]);
                int cc = compute_cost(nodes[cd].inputs[0]);
                int cd_ = compute_cost(nodes[cd].inputs[1]);
                int aoi22 = ca + cb + cc + cd_ + AOI22_COST;
                total = min(total, aoi22);
            }
        }

    } else if (node.type == NodeType::AND) {
        int a = compute_cost(node.inputs[0]);
        int b = compute_cost(node.inputs[1]);

        int nand_not = a + b + NAND2_COST + NOT_COST;
        int and2 = a + b + AND2_COST;

        total = min(nand_not, and2);
    } else if (node.type == NodeType::OR) {
        int a = compute_cost(node.inputs[0]);
        int b = compute_cost(node.inputs[1]);

        int nand_not = (a + NOT_COST) + (b + NOT_COST) + NAND2_COST;
        int or2 = a + b + OR2_COST;

        total = min(nand_not, or2);
    } else if (node.type == NodeType::OUTPUT) {
        total = compute_cost(node.inputs[0]);
    }

    node.cost = total;
    return total;
}

NodeType get_type(const string& op) {
    if (op == "AND") return NodeType::AND;
    if (op == "OR") return NodeType::OR;
    if (op == "NOT") return NodeType::NOT;
    return NodeType::INPUT;
}

void parse_netlist(const string& filepath) {
    ifstream file(filepath);
    string line;
    nodes.clear();

    while (getline(file, line)) {
        istringstream ss(line);
        string name;
        ss >> name;

        if (name.empty()) continue;

        string next;
        ss >> next;

        if (next == "INPUT") {
            nodes[name] = Node{name, NodeType::INPUT, {}};
        } else if (next == "OUTPUT") {
            output_name = name;
        } else if (next == "=") {
            string op, in1, in2;
            ss >> op >> in1;
            NodeType t = get_type(op);
            vector<string> inputs = {in1};
            if (t != NodeType::NOT) ss >> in2, inputs.push_back(in2);
            nodes[name] = Node{name, t, inputs};
        }
    }
}

int main() {
    string input_file = "input.txt";
    string output_file = "output.txt";

    parse_netlist(input_file);
    int min_cost = compute_cost(output_name);

    ofstream fout(output_file);
    fout << min_cost << endl;
    fout.close();

    cout << "Minimal cost: " << min_cost << endl;
    return 0;
}