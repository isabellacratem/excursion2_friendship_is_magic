#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>
#include <limits>

using namespace std;

enum GateType { INPUT, OUTPUT, AND, OR, NOT, NAND, NOR, AOI21, AOI22, UNKNOWN };

struct Node {
    string name;
    GateType type;
    vector<shared_ptr<Node>> children;
    int minCost = -1; // Cache for minimal cost
};

string gateTypeToStr(GateType type);
void parseInput(const string& input_file);
void printTree(shared_ptr<Node> node, int depth = 0);
shared_ptr<Node> convertToNandNot(shared_ptr<Node> node);
int calculateMinCost(shared_ptr<Node> node);
void writeOutput(int cost, const string& output_file);

unordered_map<string, shared_ptr<Node>> nodes;
string outputNodeName;

GateType strToGateType(const string& s) {
    if (s == "INPUT") return INPUT;
    if (s == "OUTPUT") return OUTPUT;
    if (s == "AND") return AND;
    if (s == "OR") return OR;
    if (s == "NOT") return NOT;
    if (s == "NAND") return NAND;
    if (s == "NOR") return NOR;
    return UNKNOWN;
}

int main() {
    parseInput("yuck_file.txt");
    
    if (nodes.find("F") == nodes.end()) {
        cerr << "F node was not constructed correctly." << endl;
        return 1;
    }
    
    cout << "Original tree:\n";
    printTree(nodes["F"], 0);
    
    cout << "\nConverting to NAND-NOT tree...\n";
    auto nandTree = convertToNandNot(nodes["F"]);
    
    cout << "Printing NAND-NOT tree:\n";
    printTree(nandTree, 0);
    
    int minCost = calculateMinCost(nandTree);
    cout << "\nMinimal cost: " << minCost << endl;
    
    writeOutput(minCost, "output.txt");
    
    return 0;
}

void writeOutput(int cost, const string& output_file) {
    ofstream file(output_file);
    if (!file) {
        cerr << "Could not open output file: " << output_file << endl;
        exit(1);
    }
    file << cost;
    file.close();
    cout << "Cost written to " << output_file << endl;
}

void parseInput(const string& input_file) {
    ifstream file(input_file);
    string line;

    if (!file) {
        cerr << "Could not open input file: " << input_file << endl;
        exit(1);
    }

    while (getline(file, line)) {
        istringstream iss(line);
        string a, b, c;

        if (!(iss >> a)) continue;

        // Handle input declarations like "a INPUT"
        if (line.find("INPUT") != string::npos) {
            nodes[a] = make_shared<Node>(Node{a, INPUT, {}});
        }
        // Handle output declaration like "F OUTPUT"
        else if (line.find("OUTPUT") != string::npos) {
            outputNodeName = a;
        }
        // Handle gate definitions like "t1 = AND b c"
        else if (line.find('=') != string::npos) {
            string target, equals, gate, in1, in2;
            istringstream gateLine(line);
            gateLine >> target >> equals >> gate >> in1;

            auto newNode = make_shared<Node>();
            newNode->name = target;
            newNode->type = strToGateType(gate);

            // Ensure child nodes exist
            if (nodes.find(in1) == nodes.end())
                nodes[in1] = make_shared<Node>(Node{in1, INPUT, {}});
            newNode->children.push_back(nodes[in1]);

            // Try to read second input if present (not for NOT)
            if (gateLine >> in2) {
                if (nodes.find(in2) == nodes.end())
                    nodes[in2] = make_shared<Node>(Node{in2, INPUT, {}});
                newNode->children.push_back(nodes[in2]);
            }

            nodes[target] = newNode;
        }
    }

    // Make sure "F" is assigned correctly
    if (!outputNodeName.empty() && nodes.find(outputNodeName) != nodes.end()) {
        nodes["F"] = nodes[outputNodeName];
    } else {
        cerr << "Error: Could not identify or find output node." << endl;
    }

    cout << "Parsing completed. Total nodes: " << nodes.size() << endl;
}

// Convert a gate to its NAND-NOT representation
shared_ptr<Node> convertToNandNot(shared_ptr<Node> node) {
    if (!node) return nullptr;
    
    // Base cases
    if (node->type == INPUT) {
        return node;
    }
    if (node->type == OUTPUT) {
        auto convertedChild = convertToNandNot(node->children[0]);
        auto outputNode = make_shared<Node>();
        outputNode->name = node->name;
        outputNode->type = OUTPUT;
        outputNode->children = {convertedChild};
        return outputNode;
    }
    
    // Handle direct NAND pattern
    if (node->type == NAND) {
        auto childA = convertToNandNot(node->children[0]);
        auto childB = convertToNandNot(node->children[1]);
        
        auto nandNode = make_shared<Node>();
        nandNode->name = "NAND(" + childA->name + "," + childB->name + ")";
        nandNode->type = NAND;
        nandNode->children = {childA, childB};
        return nandNode;
    }
    
    // Handle direct NOT pattern
    if (node->type == NOT) {
        auto child = convertToNandNot(node->children[0]);
        
        // Special case: NOT(AND) -> NAND
        if (node->children[0]->type == AND) {
            auto childA = convertToNandNot(node->children[0]->children[0]);
            auto childB = convertToNandNot(node->children[0]->children[1]);
            
            auto nandNode = make_shared<Node>();
            nandNode->name = "NAND(" + childA->name + "," + childB->name + ")";
            nandNode->type = NAND;
            nandNode->children = {childA, childB};
            return nandNode;
        }
        
        // Special case: NOT(OR) -> NAND(NOT, NOT)
        if (node->children[0]->type == OR) {
            auto childA = convertToNandNot(node->children[0]->children[0]);
            auto childB = convertToNandNot(node->children[0]->children[1]);
            
            auto notA = make_shared<Node>();
            notA->name = "NOT(" + childA->name + ")";
            notA->type = NOT;
            notA->children = {childA};
            
            auto notB = make_shared<Node>();
            notB->name = "NOT(" + childB->name + ")";
            notB->type = NOT;
            notB->children = {childB};
            
            auto nandNode = make_shared<Node>();
            nandNode->name = "NAND(" + notA->name + "," + notB->name + ")";
            nandNode->type = NAND;
            nandNode->children = {notA, notB};
            return nandNode;
        }
        
        // Special case: NOT(NOT(x)) -> x
        if (node->children[0]->type == NOT) {
            return convertToNandNot(node->children[0]->children[0]);
        }
        
        // Standard NOT
        auto notNode = make_shared<Node>();
        notNode->name = "NOT(" + child->name + ")";
        notNode->type = NOT;
        notNode->children = {child};
        return notNode;
    }
    
    // Handle AND -> NOT(NAND)
    if (node->type == AND) {
        auto childA = convertToNandNot(node->children[0]);
        auto childB = convertToNandNot(node->children[1]);
        
        auto nandNode = make_shared<Node>();
        nandNode->name = "NAND(" + childA->name + "," + childB->name + ")";
        nandNode->type = NAND;
        nandNode->children = {childA, childB};
        
        auto notNode = make_shared<Node>();
        notNode->name = "NOT(" + nandNode->name + ")";
        notNode->type = NOT;
        notNode->children = {nandNode};
        return notNode;
    }
    
    // Handle OR -> NAND(NOT, NOT)
    if (node->type == OR) {
        auto childA = convertToNandNot(node->children[0]);
        auto childB = convertToNandNot(node->children[1]);
        
        auto notA = make_shared<Node>();
        notA->name = "NOT(" + childA->name + ")";
        notA->type = NOT;
        notA->children = {childA};
        
        auto notB = make_shared<Node>();
        notB->name = "NOT(" + childB->name + ")";
        notB->type = NOT;
        notB->children = {childB};
        
        auto nandNode = make_shared<Node>();
        nandNode->name = "NAND(" + notA->name + "," + notB->name + ")";
        nandNode->type = NAND;
        nandNode->children = {notA, notB};
        return nandNode;
    }
    
    // Handle NOR -> NOT(OR) -> NOT(NAND(NOT, NOT))
    if (node->type == NOR) {
        auto childA = convertToNandNot(node->children[0]);
        auto childB = convertToNandNot(node->children[1]);
        
        auto notA = make_shared<Node>();
        notA->name = "NOT(" + childA->name + ")";
        notA->type = NOT;
        notA->children = {childA};
        
        auto notB = make_shared<Node>();
        notB->name = "NOT(" + childB->name + ")";
        notB->type = NOT;
        notB->children = {childB};
        
        auto nandNode = make_shared<Node>();
        nandNode->name = "NAND(" + notA->name + "," + notB->name + ")";
        nandNode->type = NAND;
        nandNode->children = {notA, notB};
        
        auto notNode = make_shared<Node>();
        notNode->name = "NOT(" + nandNode->name + ")";
        notNode->type = NOT;
        notNode->children = {nandNode};
        return notNode;
    }
    
    cerr << "Unsupported gate type during conversion: " << node->name << endl;
    return nullptr;
}

// Calculate the minimal cost by considering all technology options
int calculateMinCost(shared_ptr<Node> node) {
    if (!node) return 0;
    
    // If cost is already calculated, return it
    if (node->minCost != -1) return node->minCost;
    
    // Base case: input nodes have cost 0
    if (node->type == INPUT) {
        node->minCost = 0;
        return 0;
    }
    
    // For OUTPUT, the cost is the cost of its child
    if (node->type == OUTPUT) {
        node->minCost = calculateMinCost(node->children[0]);
        return node->minCost;
    }
    
    // Calculate minimal costs for all children
    for (auto& child : node->children) {
        calculateMinCost(child);
    }
    
    // Now consider different ways to implement this node
    int minCost = numeric_limits<int>::max();
    
    // Identify the pattern at this node
    if (node->type == NOT) {
        // 1. Use NOT gate directly (cost = 2)
        minCost = 2 + node->children[0]->minCost;
    }
    else if (node->type == NAND && node->children.size() == 2) {
        // 1. Use NAND2 directly (cost = 3)
        int nandCost = 3 + node->children[0]->minCost + node->children[1]->minCost;
        minCost = min(minCost, nandCost);
        
        // Check for AOI21 pattern: NAND(a, NAND(b, c))
        if (node->children[1]->type == NAND && node->children[1]->children.size() == 2) {
            int aoi21Cost = 7 + node->children[0]->minCost + 
                            node->children[1]->children[0]->minCost + 
                            node->children[1]->children[1]->minCost;
            minCost = min(minCost, aoi21Cost);
        }
        
        // Check for AOI21 pattern: NAND(NAND(a, b), c)
        if (node->children[0]->type == NAND && node->children[0]->children.size() == 2) {
            int aoi21Cost = 7 + node->children[1]->minCost + 
                            node->children[0]->children[0]->minCost + 
                            node->children[0]->children[1]->minCost;
            minCost = min(minCost, aoi21Cost);
        }
        
        // Check for AOI22 pattern: NAND(NAND(a, b), NAND(c, d))
        if (node->children[0]->type == NAND && node->children[0]->children.size() == 2 &&
            node->children[1]->type == NAND && node->children[1]->children.size() == 2) {
            int aoi22Cost = 7 + node->children[0]->children[0]->minCost + 
                            node->children[0]->children[1]->minCost +
                            node->children[1]->children[0]->minCost + 
                            node->children[1]->children[1]->minCost;
            minCost = min(minCost, aoi22Cost);
        }
    }
    else {
        // Fallback for any other pattern
        if (node->type == NAND) {
            minCost = 3; // NAND2 cost
        } else {
            minCost = 2; // Default to NOT cost
        }
        
        // Add children costs
        for (auto& child : node->children) {
            minCost += child->minCost;
        }
    }
    
    node->minCost = minCost;
    return minCost;
}

void printTree(shared_ptr<Node> node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; ++i) cout << "  ";
    cout << node->name << " [" << gateTypeToStr(node->type) << "]" << endl;

    for (auto child : node->children) {
        printTree(child, depth + 1);
    }
}

string gateTypeToStr(GateType type) {
    switch(type) {
        case INPUT: return "INPUT";
        case OUTPUT: return "OUTPUT";
        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";
        case NAND: return "NAND";
        case NOR: return "NOR";
        case AOI21: return "AOI21";
        case AOI22: return "AOI22";
        default: return "UNKNOWN";
    }
}