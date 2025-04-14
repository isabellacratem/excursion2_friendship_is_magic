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

// Structure to represent a node in the circuit
struct Node {
    std::string name;
    NodeType type;
    std::vector<std::string> inputs;
    int cost;
    bool visited;
    
    Node() : type(NodeType::INPUT), cost(-1), visited(false) {}
};

// Cost of each component in the technology library
const int NOT_COST = 2;
const int NAND2_COST = 3;
const int AND2_COST = 4;
const int NOR2_COST = 6;
const int OR2_COST = 4;
const int AOI21_COST = 7;
const int AOI22_COST = 7;

// Function to trim whitespace from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

class TechnologyMapper {
private:
    std::unordered_map<std::string, Node> nodes;
    std::string outputNode;

public:
    // Read the netlist from input file
    bool readNetlist(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Unable to open input file" << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line.find("Test") == 0 || line.find("Script") == 0) continue;

            std::istringstream iss(line);
            std::string nodeName;
            iss >> nodeName;
            
            std::string operation;
            iss >> operation;
            
            if (operation == "INPUT") {
                // This is an input node
                Node inputNode;
                inputNode.name = nodeName;
                inputNode.type = NodeType::INPUT;
                inputNode.cost = 0; // Input nodes have zero cost
                nodes[nodeName] = inputNode;
            } else if (operation == "OUTPUT") {
                // This is the output node
                outputNode = nodeName;
                // Make sure we have a node defined for the output
                if (nodes.find(nodeName) == nodes.end()) {
                    Node outNode;
                    outNode.name = nodeName;
                    outNode.type = NodeType::OUTPUT;
                    nodes[nodeName] = outNode;
                }
            } else if (operation == "=") {
                // This is a gate definition
                std::string gateType;
                iss >> gateType;
                
                Node node;
                node.name = nodeName;
                
                if (gateType == "NOT") {
                    node.type = NodeType::NOT;
                    std::string input;
                    iss >> input;
                    node.inputs.push_back(input);
                } else if (gateType == "AND") {
                    node.type = NodeType::AND;
                    std::string input1, input2;
                    iss >> input1 >> input2;
                    node.inputs.push_back(input1);
                    node.inputs.push_back(input2);
                } else if (gateType == "OR") {
                    node.type = NodeType::OR;
                    std::string input1, input2;
                    iss >> input1 >> input2;
                    node.inputs.push_back(input1);
                    node.inputs.push_back(input2);
                } else if (gateType == "NAND2") {
                    node.type = NodeType::NAND2;
                    std::string input1, input2;
                    iss >> input1 >> input2;
                    node.inputs.push_back(input1);
                    node.inputs.push_back(input2);
                } else if (gateType == "NOR2") {
                    node.type = NodeType::NOR2;
                    std::string input1, input2;
                    iss >> input1 >> input2;
                    node.inputs.push_back(input1);
                    node.inputs.push_back(input2);
                } else if (gateType == "AOI21") {
                    node.type = NodeType::AOI21;
                    std::string input1, input2, input3;
                    iss >> input1 >> input2 >> input3;
                    node.inputs.push_back(input1);
                    node.inputs.push_back(input2);
                    node.inputs.push_back(input3);
                } else if (gateType == "AOI22") {
                    node.type = NodeType::AOI22;
                    std::string input1, input2, input3, input4;
                    iss >> input1 >> input2 >> input3 >> input4;
                    node.inputs.push_back(input1);
                    node.inputs.push_back(input2);
                    node.inputs.push_back(input3);
                    node.inputs.push_back(input4);
                } else {
                    // Important fix: Don't treat unknown gate types as errors since they could be node references
                    if (iss.peek() == EOF) {
                        // This is a simple assignment F = t5
                        node.name = nodeName;
                        // Use OUTPUT as a reference type
                        node.type = NodeType::OUTPUT;
                        node.inputs.push_back(gateType);
                    } else {
                        // This is an error case
                        std::cerr << "Error: Invalid gate type in line: " << line << std::endl;
                        file.close();
                        return false;
                    }
                }
                
                nodes[nodeName] = node;
            } else {
                std::cerr << "Error: Invalid syntax in line: " << line << std::endl;
                file.close();
                return false;
            }
        }
        
        file.close();
        
        // Verify that we have an output node
        if (outputNode.empty() || nodes.find(outputNode) == nodes.end()) {
            std::cerr << "Error: Output node not properly defined" << std::endl;
            return false;
        }
        
        return true;
    }
    
    // Calculate the minimal cost of the NAND-NOT transformed tree
    int calculateMinimalCost() {
        if (nodes.find(outputNode) == nodes.end()) {
            std::cerr << "Error: Output node not found: " << outputNode << std::endl;
            return -1;
        }
        
        // Reset all nodes' visited status
        for (auto& pair : nodes) {
            pair.second.visited = false;
            pair.second.cost = -1;  // Reset cost too
        }
        
        return calculateNodeCost(outputNode);
    }
    
private:
    // Recursively calculate the cost of a node
    int calculateNodeCost(const std::string& nodeName) {
        // Check if node exists
        if (nodes.find(nodeName) == nodes.end()) {
            std::cerr << "Error: Node " << nodeName << " not found" << std::endl;
            return -1;
        }
        
        Node& node = nodes[nodeName];
        
        // If we've already computed the cost, return it
        if (node.visited && node.cost >= 0) {
            return node.cost;
        }
        
        // Mark node as visited
        node.visited = true;
        
        // Input nodes have zero cost
        if (node.type == NodeType::INPUT) {
            node.cost = 0;
            return 0;
        }
        
        // For OUTPUT nodes, we need to find what they're connected to
        if (node.type == NodeType::OUTPUT) {
            if (!node.inputs.empty()) {
                node.cost = calculateNodeCost(node.inputs[0]);
            } else {
                // This is an error, output should be connected to something
                std::cerr << "Error: Output node " << nodeName << " has no inputs" << std::endl;
                return -1;
            }
            return node.cost;
        }
        
        // Calculate costs for input nodes first
        int inputCost = 0;
        for (const auto& input : node.inputs) {
            int cost = calculateNodeCost(input);
            if (cost < 0) return -1; // Error occurred
            inputCost += cost;
        }
        
        // Calculate the cost based on the node type
        if (node.type == NodeType::NOT) {
            // NOT gate can be implemented directly or we can use a NAND with tied inputs
            node.cost = std::min(NOT_COST, NAND2_COST) + inputCost;
        }
        else if (node.type == NodeType::AND) {
            // For AND:
            // 1. Direct AND implementation
            int costWithAND = AND2_COST + inputCost;
            
            // 2. Using NAND followed by NOT
            int costWithNAND_NOT = NAND2_COST + NOT_COST + inputCost;
            
            // 3. Using NAND with tied inputs (for single input) followed by NOT
            int costWithNAND_TiedInputs = (node.inputs.size() == 1) ? 
                NAND2_COST + NOT_COST + inputCost : std::numeric_limits<int>::max();
            
            node.cost = std::min({costWithAND, costWithNAND_NOT, costWithNAND_TiedInputs});
        }
        else if (node.type == NodeType::OR) {
            // For OR:
            // 1. Direct OR implementation
            int costWithOR = OR2_COST + inputCost;
            
            // 2. Using NOR followed by NOT
            int costWithNOR_NOT = NOR2_COST + NOT_COST + inputCost;
            
            // 3. Using De Morgan's laws: A OR B = NOT(NOT(A) AND NOT(B))
            int costWithDeMorgan = 2 * NOT_COST + NAND2_COST + inputCost;
            
            node.cost = std::min({costWithOR, costWithNOR_NOT, costWithDeMorgan});
        }
        else if (node.type == NodeType::NAND2) {
            // NAND2 gate directly
            node.cost = NAND2_COST + inputCost;
        }
        else if (node.type == NodeType::NOR2) {
            // For NOR2:
            // 1. Direct NOR2 implementation
            int costWithNOR = NOR2_COST + inputCost;
            
            // 2. Using De Morgan's laws and NAND: NOR(A,B) = NOT(A OR B) = NOT(NOT(NOT(A) AND NOT(B)))
            int costWithDeMorgan = 3 * NOT_COST + NAND2_COST + inputCost;
            
            node.cost = std::min(costWithNOR, costWithDeMorgan);
        }
        else if (node.type == NodeType::AOI21) {
            // For AOI21 (AND-OR-INVERT with 2 inputs AND, 1 input OR):
            node.cost = AOI21_COST + inputCost;
        }
        else if (node.type == NodeType::AOI22) {
            // For AOI22 (AND-OR-INVERT with 2x2 inputs):
            node.cost = AOI22_COST + inputCost;
        }
        else {
            std::cerr << "Error: Unknown node type for " << nodeName << std::endl;
            return -1;
        }
        
        return node.cost;
    }
    
public:
    // Debug function to print all nodes and their costs
    void printNodes() {
        std::cout << "Nodes in the netlist:" << std::endl;
        for (const auto& pair : nodes) {
            std::cout << pair.first << " (";
            switch (pair.second.type) {
                case NodeType::INPUT: std::cout << "INPUT"; break;
                case NodeType::AND: std::cout << "AND"; break;
                case NodeType::OR: std::cout << "OR"; break;
                case NodeType::NOT: std::cout << "NOT"; break;
                case NodeType::OUTPUT: std::cout << "OUTPUT"; break;
                case NodeType::NAND2: std::cout << "NAND2"; break;
                case NodeType::NOR2: std::cout << "NOR2"; break;
                case NodeType::AOI21: std::cout << "AOI21"; break;
                case NodeType::AOI22: std::cout << "AOI22"; break;
                default: std::cout << "UNKNOWN"; break;
            }
            std::cout << ") cost: " << pair.second.cost;
            if (!pair.second.inputs.empty()) {
                std::cout << " inputs: ";
                for (const auto& input : pair.second.inputs) {
                    std::cout << input << " ";
                }
            }
            std::cout << std::endl;
        }
        std::cout << "Output node: " << outputNode << std::endl;
    }
};

int main() {
    TechnologyMapper mapper;
    
    // Read the netlist
    if (!mapper.readNetlist("input.txt")) {
        std::cerr << "Failed to read netlist" << std::endl;
        return 1;
    }
    
    // Calculate the minimal cost
    int minimalCost = mapper.calculateMinimalCost();
    
    if (minimalCost < 0) {
        std::cerr << "Error calculating minimal cost" << std::endl;
        return 1;
    }
    
    // Uncomment for debugging
    // mapper.printNodes();
    
    // Write the result to output file
    std::ofstream outFile("output.txt");
    if (!outFile.is_open()) {
        std::cerr << "Error: Unable to open output file" << std::endl;
        return 1;
    }
    
    outFile << minimalCost;
    outFile.close();
    
    std::cout << "Minimal cost: " << minimalCost << std::endl;
    
    return 0;
}