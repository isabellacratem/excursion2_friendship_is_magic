#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

enum class NodeType { INPUT, AND, OR, NOT };

struct Node {
    string name;
    NodeType type;
    vector<string> inputs;
    int cost = -1;
    bool visited = false;
};

unordered_map<string, Node> nodes;
unordered_map<string, int> cost_map = { {"NOT", 2}, {"NAND2", 3} };

int compute_cost(const string& name) {
    if (nodes[name].visited) return nodes[name].cost;
    nodes[name].visited = true;

    Node& node = nodes[name];
    if (node.type == NodeType::INPUT) return node.cost = 0;

    if (node.type == NodeType::NOT) {
        return node.cost = compute_cost(node.inputs[0]) + cost_map["NOT"];
    }
    if (node.type == NodeType::AND) {
        int a = compute_cost(node.inputs[0]);
        int b = compute_cost(node.inputs[1]);
        return node.cost = a + b + cost_map["NAND2"] + cost_map["NOT"];
    }
    if (node.type == NodeType::OR) {
        int a = compute_cost(node.inputs[0]) + cost_map["NOT"];
        int b = compute_cost(node.inputs[1]) + cost_map["NOT"];
        return node.cost = a + b + cost_map["NAND2"] + cost_map["NOT"];
    }
    return 0;
}

NodeType parse_type(const string& s) {
    if (s == "AND") return NodeType::AND;
    if (s == "OR") return NodeType::OR;
    if (s == "NOT") return NodeType::NOT;
    return NodeType::INPUT;
}

void parse_netlist(const string& filename, string& output_node) {
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        line.erase(remove(line.begin(), line.end(), '\r'), line.end());
        if (line.empty()) continue;

        stringstream ss(line);
        string lhs, eq, op, in1, in2;

        ss >> lhs;
        if (lhs == "INPUT" || lhs == "OUTPUT") continue;

        ss >> eq;
        if (eq == "=") {
            ss >> op;
            Node node;
            node.name = lhs;
            node.type = parse_type(op);
            if (op == "NOT") {
                ss >> in1;
                node.inputs.push_back(in1);
            } else {
                ss >> in1 >> in2;
                node.inputs.push_back(in1);
                node.inputs.push_back(in2);
            }
            nodes[lhs] = node;
        } else if (eq == "INPUT") {
            Node node;
            node.name = lhs;
            node.type = NodeType::INPUT;
            nodes[lhs] = node;
        } else if (eq == "OUTPUT") {
            output_node = lhs;
        }
    }
}

int main() {
    string output_node;
    parse_netlist("input.txt", output_node);

    int result = compute_cost(output_node);

    ofstream out("output.txt");
    out << result << endl;
    out.close();

    cout << "Minimum cost: " << result << endl;
    return 0;
}
