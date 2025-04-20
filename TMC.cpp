#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
using namespace std;

// Node types in the circuit
enum NodeType { INPUT, OUTPUT, AND, OR, NOT, UNKNOWN };

// Structure to hold gate info
struct Node {
    NodeType type;
    vector<string> inputs;
    int cost;
    bool visited;
    
    Node() : type(UNKNOWN), cost(-1), visited(false) {}
};

// Gate costs in the technology library
const int NOT_COST = 2;
const int NAND2_COST = 3;
const int AND2_COST = 4;
const int OR2_COST = 4;
const int NOR2_COST = 6;
const int AOI21_COST = 7;
const int AOI22_COST = 7;

// Converts string to node type
NodeType getNodeType(const string& s) {
    if (s == "AND") return AND;
    if (s == "OR") return OR;
    if (s == "NOT") return NOT;
    if (s == "INPUT") return INPUT;
    if (s == "OUTPUT") return OUTPUT;
    return UNKNOWN;
}

// Parses the input netlist
bool readNetlist(const string& filename, unordered_map<string, Node>& circuit, string& outputNode) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return false;
    }
    
    string line;
    while (getline(file, line)) {
        // Skip test case header lines
        if (line.empty() || line.find("Test") == 0 || line.find("Script") == 0) {
            continue;
        }
        
        stringstream ss(line);
        string name;
        ss >> name;

        string typeOrEqual;
        ss >> typeOrEqual;

        if (typeOrEqual == "INPUT") {
            Node& node = circuit[name];
            node.type = INPUT;
            node.cost = 0;
        } else if (typeOrEqual == "OUTPUT") {
            outputNode = name;
        } else if (typeOrEqual == "=") {
            string gateTypeStr;
            ss >> gateTypeStr;

            NodeType gateType = getNodeType(gateTypeStr);
            vector<string> inputs;
            string inputName;
            while (ss >> inputName) {
                inputs.push_back(inputName);
            }

            Node& node = circuit[name];
            node.type = gateType;
            node.inputs = inputs;
        }
    }

    file.close();
    return !outputNode.empty();
}

// Main evaluation function - calculates minimal cost for each node
int evaluate(const string& nodeName, unordered_map<string, Node>& circuit) {
    Node& node = circuit[nodeName];
    
    // Return memoized result if available
    if (node.visited && node.cost >= 0) {
        return node.cost;
    }
    
    // Mark as visited to avoid infinite recursion
    node.visited = true;
    
    // Base cases
    if (node.type == INPUT) {
        node.cost = 0;
        return 0;
    }
    
    if (node.type == OUTPUT) {
        node.cost = evaluate(node.inputs[0], circuit);
        return node.cost;
    }
    
    // --- Special Pattern Recognition ---
    
    // NOT patterns
    if (node.type == NOT) {
        const string& input = node.inputs[0];
        
        // Double negation: NOT(NOT(x)) -> x
        if (circuit[input].type == NOT) {
            int cost = evaluate(circuit[input].inputs[0], circuit);
            if (cost >= 0) {
                node.cost = cost;
                return cost;
            }
        }
        
        // NOT(OR(a,b)) -> NOR2(a,b)
        if (circuit[input].type == OR && circuit[input].inputs.size() == 2) {
            auto& inputs = circuit[input].inputs;
            int cost1 = evaluate(inputs[0], circuit);
            int cost2 = evaluate(inputs[1], circuit);
            if (cost1 >= 0 && cost2 >= 0) {
                int norCost = cost1 + cost2 + NOR2_COST;
                node.cost = norCost;
                return norCost;
            }
        }
        
        // NOT(AND(a,b)) -> NAND2(a,b)
        if (circuit[input].type == AND && circuit[input].inputs.size() == 2) {
            auto& inputs = circuit[input].inputs;
            int cost1 = evaluate(inputs[0], circuit);
            int cost2 = evaluate(inputs[1], circuit);
            if (cost1 >= 0 && cost2 >= 0) {
                int nandCost = cost1 + cost2 + NAND2_COST;
                node.cost = nandCost;
                return nandCost;
            }
        }
        
        // AOI21 pattern: NOT(OR(AND(a,b),c))
        if (circuit[input].type == OR && circuit[input].inputs.size() == 2) {
            auto& orInputs = circuit[input].inputs;
            bool and0 = (circuit.count(orInputs[0]) > 0 && circuit[orInputs[0]].type == AND);
            bool and1 = (circuit.count(orInputs[1]) > 0 && circuit[orInputs[1]].type == AND);
            
            // Case 1: AND + non-AND
            if (and0 && !and1) {
                auto& andInputs = circuit[orInputs[0]].inputs;
                int costA = evaluate(andInputs[0], circuit);
                int costB = evaluate(andInputs[1], circuit);
                int costC = evaluate(orInputs[1], circuit);
                if (costA >= 0 && costB >= 0 && costC >= 0) {
                    int aoi21Cost = costA + costB + costC + AOI21_COST;
                    node.cost = aoi21Cost;
                    return aoi21Cost;
                }
            }
            
            // Case 2: non-AND + AND
            if (!and0 && and1) {
                auto& andInputs = circuit[orInputs[1]].inputs;
                int costA = evaluate(andInputs[0], circuit);
                int costB = evaluate(andInputs[1], circuit);
                int costC = evaluate(orInputs[0], circuit);
                if (costA >= 0 && costB >= 0 && costC >= 0) {
                    int aoi21Cost = costA + costB + costC + AOI21_COST;
                    node.cost = aoi21Cost;
                    return aoi21Cost;
                }
            }
            
            // Case 3: AND + AND (AOI22 pattern)
            if (and0 && and1) {
                auto& andInputs0 = circuit[orInputs[0]].inputs;
                auto& andInputs1 = circuit[orInputs[1]].inputs;
                int costA = evaluate(andInputs0[0], circuit);
                int costB = evaluate(andInputs0[1], circuit);
                int costC = evaluate(andInputs1[0], circuit);
                int costD = evaluate(andInputs1[1], circuit);
                if (costA >= 0 && costB >= 0 && costC >= 0 && costD >= 0) {
                    int aoi22Cost = costA + costB + costC + costD + AOI22_COST;
                    node.cost = aoi22Cost;
                    return aoi22Cost;
                }
            }
        }
    }
    
    // AND Pattern: AND(AND(a,b), NOT(OR(c,d))) or AND(NOT(OR(c,d)), AND(a,b))
    if (node.type == AND && node.inputs.size() == 2) {
        auto& inputs = node.inputs;
        
        // Check for pattern: AND(AND(a,b), NOT(OR(c,d)))
        if (circuit[inputs[0]].type == AND && circuit[inputs[1]].type == NOT && 
            circuit.count(circuit[inputs[1]].inputs[0]) > 0 && circuit[circuit[inputs[1]].inputs[0]].type == OR) {
            
            auto& andInputs = circuit[inputs[0]].inputs;
            auto& orInputs = circuit[circuit[inputs[1]].inputs[0]].inputs;
            
            int costA = evaluate(andInputs[0], circuit);
            int costB = evaluate(andInputs[1], circuit);
            int costC = evaluate(orInputs[0], circuit);
            int costD = evaluate(orInputs[1], circuit);
            
            if (costA >= 0 && costB >= 0 && costC >= 0 && costD >= 0) {
                int nandCost = costA + costB + NAND2_COST;
                int orCost = costC + costD + OR2_COST;
                int norCost = nandCost + orCost + NOR2_COST;
                node.cost = norCost;
                return norCost;
            }
        }
        
        // Check for pattern: AND(NOT(OR(c,d)), AND(a,b))
        if (circuit[inputs[1]].type == AND && circuit[inputs[0]].type == NOT && 
            circuit.count(circuit[inputs[0]].inputs[0]) > 0 && circuit[circuit[inputs[0]].inputs[0]].type == OR) {
            
            auto& andInputs = circuit[inputs[1]].inputs;
            auto& orInputs = circuit[circuit[inputs[0]].inputs[0]].inputs;
            
            int costA = evaluate(andInputs[0], circuit);
            int costB = evaluate(andInputs[1], circuit);
            int costC = evaluate(orInputs[0], circuit);
            int costD = evaluate(orInputs[1], circuit);
            
            if (costA >= 0 && costB >= 0 && costC >= 0 && costD >= 0) {
                int nandCost = costA + costB + NAND2_COST;
                int orCost = costC + costD + OR2_COST;
                int norCost = nandCost + orCost + NOR2_COST;
                node.cost = norCost;
                return norCost;
            }
        }
    }
    
    // --- Standard Gate Implementations ---
    
    // Calculate costs for inputs
    int inputCostSum = 0;
    for (const string& input : node.inputs) {
        int inputCost = evaluate(input, circuit);
        if (inputCost < 0) return -1;  // Invalid
        inputCostSum += inputCost;
    }
    
    int minCost = numeric_limits<int>::max();
    
    switch (node.type) {
        case NOT:
            minCost = min(inputCostSum + NOT_COST, inputCostSum + NAND2_COST);  // NOT or NAND with tied inputs
            break;
            
        case AND:
            if (node.inputs.size() == 2) {
                // Direct AND2 implementation
                int and2Cost = inputCostSum + AND2_COST;
                
                // NAND2 followed by NOT
                int nandNotCost = inputCostSum + NAND2_COST + NOT_COST;
                
                minCost = min(and2Cost, nandNotCost);
            } else {
                // For multi-input AND gates, decompose into 2-input gates
                minCost = inputCostSum + (node.inputs.size() - 1) * AND2_COST;
            }
            break;
            
        case OR:
            if (node.inputs.size() == 2) {
                // Direct OR2 implementation
                int or2Cost = inputCostSum + OR2_COST;
                
                // NOR2 followed by NOT
                int norNotCost = inputCostSum + NOR2_COST + NOT_COST;
                
                // NOT on each input followed by NAND2
                int notNotNandCost = inputCostSum + 2 * NOT_COST + NAND2_COST;
                
                minCost = min({or2Cost, norNotCost, notNotNandCost});
            } else {
                // For multi-input OR gates, decompose into 2-input gates
                minCost = inputCostSum + (node.inputs.size() - 1) * OR2_COST;
            }
            break;
            
        default:
            return -1;  // Invalid type
    }
    
    node.cost = minCost;
    return minCost;
}

int main(int argc, char* argv[]) {
    string inputFile = "input2.txt";
    if (argc > 1) {
        inputFile = argv[1];
    }
    
    unordered_map<string, Node> circuit;
    string outputNode;

    // Read and parse the netlist
    if (!readNetlist(inputFile, circuit, outputNode)) {
        cerr << "Failed to read or parse netlist!" << endl;
        return 1;
    }

    // Reset visited flags and costs
    for (auto& pair : circuit) {
        pair.second.visited = false;
        pair.second.cost = -1;
    }
    
    // Calculate the minimal cost
    int cost = evaluate(outputNode, circuit);
    
    if (cost < 0) {
        cerr << "Error: Could not calculate valid cost!" << endl;
        return 1;
    }

    // Output the result
    ofstream outFile("output.txt");
    outFile << cost << endl;
    outFile.close();

    cout << "Minimal cost = " << cost << endl;

    return 0;
}