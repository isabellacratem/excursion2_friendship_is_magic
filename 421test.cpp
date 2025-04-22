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
unordered_map<string, int> memo;
string output_name;

// NAND-NOT only cost library
const int NOT_COST = 2;
const int NAND2_COST = 3;

int compute_nand_not_cost(const string& name) {
    if (memo.count(name)) return memo[name];
    if (nodes.find(name) == nodes.end()) {
        cerr << "Error: Undefined node '" << name << "' used as input." << endl;
        exit(1);
    }

    Node& node = nodes[name];
    if (node.type == NodeType::INPUT) {
        memo[name] = 0;
        return 0;
    }

    int total = 0;
    if (node.type == NodeType::NOT && node.inputs.size() == 1) {
        compute_nand_not_cost(node.inputs[0]);
        total = NOT_COST;  // NAND(x, x)
    } else if (node.type == NodeType::AND && node.inputs.size() == 2) {
        compute_nand_not_cost(node.inputs[0]);
        compute_nand_not_cost(node.inputs[1]);
        total = NAND2_COST + NOT_COST;  // NOT(NAND(a, b))
    } else if (node.type == NodeType::OR && node.inputs.size() == 2) {
        compute_nand_not_cost(node.inputs[0]);
        compute_nand_not_cost(node.inputs[1]);
        total = 2 * NOT_COST + NAND2_COST;  // NAND(NOT a, NOT b)
    } else if (node.type == NodeType::OUTPUT && node.inputs.size() == 1) {
        total = compute_nand_not_cost(node.inputs[0]);
    }

    memo[name] = total;
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
    memo.clear();

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
    string input_file = "input8.txt";
    string output_file = "output.txt";

    parse_netlist(input_file);
    int min_cost = compute_nand_not_cost(output_name);

    ofstream fout(output_file);
    fout << min_cost << endl;
    fout.close();

    cout << "Minimal cost: " << min_cost << endl;
    return 0;
}