#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
using namespace std;

enum GateType { INPUT, OUTPUT, AND, OR, NOT, UNKNOWN };

// Structure to hold basic gate info from netlist
struct Node {
    GateType type;
    vector<string> inputs;
};

// Structure to build NAND-NOT tree
struct TreeNode {
    string name;
    string gate;                   // "NAND", "NOT", or "INPUT"
    vector<TreeNode*> inputs;     // children of this gate
};

// Converts gate string to enum
GateType getGateType(const string& s) {
    if (s == "AND") return AND;
    if (s == "OR") return OR;
    if (s == "NOT") return NOT;
    return UNKNOWN;
}

// Parses the input netlist into a circuit map
void readNetlist(const string& filename, unordered_map<string, Node>& circuit, string& outputName) {
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        string name;
        ss >> name;

        string typeOrEqual;
        ss >> typeOrEqual;

        if (typeOrEqual == "INPUT") {
            circuit[name] = {INPUT, {}};
        } else if (typeOrEqual == "OUTPUT") {
            outputName = name;
        } else if (typeOrEqual == "=") {
            string gateTypeStr;
            ss >> gateTypeStr;

            GateType gateType = getGateType(gateTypeStr);
            vector<string> inputs;
            string inputName;
            while (ss >> inputName) {
                inputs.push_back(inputName);
            }

            circuit[name] = {gateType, inputs};
        }
    }

    file.close();
}

// Builds the NAND-NOT version of the logic circuit
TreeNode* buildNandNotTree(string nodeName, unordered_map<string, Node>& circuit, unordered_map<string, TreeNode*>& memo) {
    if (memo.count(nodeName)) return memo[nodeName];

    Node node = circuit[nodeName];

    if (node.type == INPUT) {
        TreeNode* inputNode = new TreeNode{nodeName, "INPUT", {}};
        memo[nodeName] = inputNode;
        return inputNode;
    }

    vector<TreeNode*> inputs;
    for (const string& inputName : node.inputs) {
        inputs.push_back(buildNandNotTree(inputName, circuit, memo));
    }

    TreeNode* result = nullptr;

    if (node.type == AND) {
        TreeNode* nandGate = new TreeNode{"", "NAND", inputs};
        result = new TreeNode{"", "NOT", {nandGate}};
    } else if (node.type == OR) {
        TreeNode* not1 = new TreeNode{"", "NOT", {inputs[0]}};
        TreeNode* not2 = new TreeNode{"", "NOT", {inputs[1]}};
        result = new TreeNode{"", "NAND", {not1, not2}};
    } else if (node.type == NOT) {
        result = new TreeNode{"", "NOT", {inputs[0]}};
    }

    memo[nodeName] = result;
    return result;
}

// Computes the cost of the NAND-NOT tree (without double-counting)
int computeCost(TreeNode* node, unordered_map<TreeNode*, int>& memo) {
    if (memo.count(node)) return memo[node]; // Already counted this node

    int cost = 0;

    for (TreeNode* child : node->inputs) {
        cost += computeCost(child, memo);
    }

    if (node->gate == "NAND") cost += 3;
    else if (node->gate == "NOT") cost += 2;

    memo[node] = cost;
    return cost;
}

// Optional: visualize tree structure (for debugging)
void printTree(TreeNode* node, int indent = 0) {
    if (!node) return;
    for (int i = 0; i < indent; i++) cout << "  ";
    cout << node->gate;
    if (!node->name.empty()) cout << " (" << node->name << ")";
    cout << endl;

    for (TreeNode* input : node->inputs) {
        printTree(input, indent + 1);
    }
}

int main() {
    unordered_map<string, Node> circuit;
    string outputNode;

    readNetlist("input2.txt", circuit, outputNode);

    // Step 1: Convert to NAND-NOT tree
    unordered_map<string, TreeNode*> treeMemo;
    TreeNode* root = buildNandNotTree(outputNode, circuit, treeMemo);

    // Step 2: Compute cost from final NAND-NOT tree
    unordered_map<TreeNode*, int> costMemo;
    int cost = computeCost(root, costMemo);

    // Step 3: Output result
    ofstream out("output.txt");
    out << cost << endl;
    out.close();

    cout << "Minimal cost = " << cost << endl;

    // Optional: print tree structure
    // printTree(root);

    return 0;
}
