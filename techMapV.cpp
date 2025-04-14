#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <climits>
using namespace std;

enum GateType { INPUT, OUTPUT, AND, OR, NOT, UNKNOWN };

struct Node {
    GateType type;
    vector<string> inputs;
};

struct TreeNode {
    string name;               // logical name for memoization
    string gate;               // "NAND", "NOT", or "INPUT"
    vector<TreeNode*> inputs; // input TreeNode pointers
};

GateType getGateType(const string& s) {
    if (s == "AND") return AND;
    if (s == "OR") return OR;
    if (s == "NOT") return NOT;
    return UNKNOWN;
}

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

TreeNode* buildNandNotTree(string name, unordered_map<string, Node>& circuit, unordered_map<string, TreeNode*>& memo) {
    if (memo.count(name)) return memo[name];
    Node& node = circuit[name];

    if (node.type == INPUT) {
        TreeNode* leaf = new TreeNode{name, "INPUT", {}};
        memo[name] = leaf;
        return leaf;
    }

    if (node.type == NOT) {
        TreeNode* child = buildNandNotTree(node.inputs[0], circuit, memo);
        TreeNode* notNode = new TreeNode{name, "NOT", {child}};
        memo[name] = notNode;
        return notNode;
    }

    TreeNode* a = buildNandNotTree(node.inputs[0], circuit, memo);
    TreeNode* b = buildNandNotTree(node.inputs[1], circuit, memo);

    if (node.type == AND) {
        TreeNode* nandNode = new TreeNode{name + "_nand", "NAND", {a, b}};
        TreeNode* notNode = new TreeNode{name, "NOT", {nandNode}};
        memo[name] = notNode;
        return notNode;
    }
    if (node.type == OR) {
        TreeNode* notA = new TreeNode{name + "_na", "NOT", {a}};
        TreeNode* notB = new TreeNode{name + "_nb", "NOT", {b}};
        TreeNode* nandNode = new TreeNode{name, "NAND", {notA, notB}};
        memo[name] = nandNode;
        return nandNode;
    }
    return nullptr;
}

int computeMinCost(TreeNode* node, unordered_map<TreeNode*, int>& dp) {
    if (dp.count(node)) return dp[node];

    int minCost = INT_MAX;

    // Base case: input
    if (node->gate == "INPUT") {
        dp[node] = 0;
        return 0;
    }

    // Try matching as NOT gate
    if (node->gate == "NOT" && node->inputs.size() == 1) {
        int childCost = computeMinCost(node->inputs[0], dp);
        minCost = min(minCost, childCost + 2);
    }

    // Try matching as NAND gate
    if (node->gate == "NAND" && node->inputs.size() == 2) {
        int leftCost = computeMinCost(node->inputs[0], dp);
        int rightCost = computeMinCost(node->inputs[1], dp);
        minCost = min(minCost, leftCost + rightCost + 3);
    }

    // Try matching as AND = NOT(NAND(x,y))
    if (node->gate == "NOT" && node->inputs.size() == 1) {
        TreeNode* nandChild = node->inputs[0];
        if (nandChild->gate == "NAND" && nandChild->inputs.size() == 2) {
            minCost = min(minCost, 4); // AND2 = 4 from tech library // match AND2 gate from tech library
        }
    }

    // Try matching as OR = NAND(NOT(x), NOT(y))
    if (node->gate == "NAND" && node->inputs.size() == 2) {
        TreeNode* n1 = node->inputs[0];
        TreeNode* n2 = node->inputs[1];
        if (n1->gate == "NOT" && n2->gate == "NOT" &&
            n1->inputs.size() == 1 && n2->inputs.size() == 1) {
            int x = computeMinCost(n1->inputs[0], dp); // t1
            int y = computeMinCost(n2->inputs[0], dp); // t2
            minCost = min(minCost, x + y + 4); // OR2 matched from tech library
        }
    }

    // Try matching AOI21 = NOT(NAND(NAND(x, y), z))
    if (node->gate == "NOT" && node->inputs.size() == 1) {
        TreeNode* nand1 = node->inputs[0];
        if (nand1->gate == "NAND" && nand1->inputs.size() == 2) {
            TreeNode* n1 = nand1->inputs[0];
            TreeNode* n2 = nand1->inputs[1];
            if (n1->gate == "NAND" && n1->inputs.size() == 2) {
                int a = computeMinCost(n1->inputs[0], dp);
                int b = computeMinCost(n1->inputs[1], dp);
                int c = computeMinCost(n2, dp);
                minCost = min(minCost, a + b + c + 7); // AOI21
            }
        }
    }

    // Try matching AOI22 = NOT(NAND(NAND(x, y), NAND(z, w)))
    if (node->gate == "NOT" && node->inputs.size() == 1) {
        TreeNode* nandTop = node->inputs[0];
        if (nandTop->gate == "NAND" && nandTop->inputs.size() == 2) {
            TreeNode* n1 = nandTop->inputs[0];
            TreeNode* n2 = nandTop->inputs[1];
            if (n1->gate == "NAND" && n1->inputs.size() == 2 &&
                n2->gate == "NAND" && n2->inputs.size() == 2) {
                int a = computeMinCost(n1->inputs[0], dp);
                int b = computeMinCost(n1->inputs[1], dp);
                int c = computeMinCost(n2->inputs[0], dp);
                int d = computeMinCost(n2->inputs[1], dp);
                minCost = min(minCost, a + b + c + d + 7); // AOI22
            }
        }
    }

    if (minCost == INT_MAX) {
        minCost = 0;
    }
    dp[node] = minCost;
    return minCost;
}

int main() {
    unordered_map<string, Node> circuit;
    string outputNode;
    readNetlist("input.txt", circuit, outputNode);

    unordered_map<string, TreeNode*> memo;
    TreeNode* root = buildNandNotTree(outputNode, circuit, memo);

    unordered_map<TreeNode*, int> dp;
    int totalCost = computeMinCost(root, dp);

    ofstream out("output.txt");
    out << totalCost << endl;
    out.close();

    cout << "Minimal cost = " << totalCost << endl;
    return 0;
}
