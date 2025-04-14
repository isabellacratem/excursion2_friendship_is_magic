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
    UNKNOWN
};

// Structure to represent a node in the circuit
struct Node {
    std::string name;
    NodeType type;
    std::vector<std::string> inputs;
    int cost;
    bool visited;
    
    Node() : type(NodeType::UNKNOWN), cost(-1), visited(false) {}
};

// Cost of each component in the technology library
const int NOT_COST = 2;
const int NAND2_COST = 3;
const int AND2_COST = 4;
const int NOR2_COST = 6;
const int OR2_COST = 4;
const int AOI21_COST = 7;
const int AOI22_COST = 7;

// Technology mapping class
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
            // Skip empty lines
            if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
                continue;
            }

            std::istringstream iss(line);
            std::string token;
            iss >> token;

            if (token == "INPUT") {
                std::string inputName;
                iss >> inputName;
                
                Node inputNode;
                inputNode.name = inputName;
                inputNode.type = NodeType::INPUT;
                inputNode.cost = 0; // Input nodes have zero cost
                nodes[inputName] = inputNode;
            }
            else if (token == "OUTPUT") {
                iss >> outputNode;
            }
            else {
                // Parse node definition
                std::string nodeName = token;
                std::string equals;
                iss >> equals; // Skip "="
                
                if (equals != "=") {
                    std::cerr << "Error: Invalid syntax in netlist" << std::endl;
                    return false;
                }
                
                std::string operation;
                iss >> operation;
                
                Node node;
                node.name = nodeName;
                
                if (operation == "NOT") {
                    node.type = NodeType::NOT;
                    std::string input;
                    iss >> input;
                    node.inputs.push_back(input);
                }
                else if (operation == "AND") {
                    node.type = NodeType::AND;
                    std::string input1, input2;
                    iss >> input1 >> input2;
                    node.inputs.push_back(input1);
                    node.inputs.push_back(input2);
                }
                else if (operation == "OR") {
                    node.type = NodeType::OR;
                    std::string input1, input2;
                    iss >> input1 >> input2;
                    node.inputs.push_back(input1);
                    node.inputs.push_back(input2);
                }
                else {
                    std::cerr << "Error: Unknown operation: " << operation << std::endl;
                    return false;
                }
                
                nodes[nodeName] = node;
            }
        }
        
        file.close();
        return true;
    }
    
    // Calculate the minimal cost of the NAND-NOT transformed tree
    int calculateMinimalCost() {
        if (nodes.find(outputNode) == nodes.end()) {
            std::cerr << "Error: Output node not found" << std::endl;
            return -1;
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
        if (node.visited) {
            return node.cost;
        }
        
        // Mark node as visited
        node.visited = true;
        
        // Input nodes have zero cost
        if (node.type == NodeType::INPUT) {
            node.cost = 0;
            return 0;
        }
        
        // Calculate costs for input nodes first
        for (const auto& input : node.inputs) {
            calculateNodeCost(input);
        }
        
        // Calculate the cost based on the node type
        if (node.type == NodeType::NOT) {
            // NOT gate can be implemented directly
            node.cost = NOT_COST + nodes[node.inputs[0]].cost;
        }
        else if (node.type == NodeType::AND) {
            // AND gate can be implemented as NAND followed by NOT
            int costWithNAND = NAND2_COST + NOT_COST + nodes[node.inputs[0]].cost + nodes[node.inputs[1]].cost;
            
            // Direct AND implementation
            int costWithAND = AND2_COST + nodes[node.inputs[0]].cost + nodes[node.inputs[1]].cost;
            
            // Check if AOI21 can be used (needs specific structure)
            int costWithAOI21 = std::numeric_limits<int>::max();
            
            // Check if AOI22 can be used (needs specific structure)
            int costWithAOI22 = std::numeric_limits<int>::max();
            
            // Choose the minimum cost
            node.cost = std::min({costWithNAND, costWithAND, costWithAOI21, costWithAOI22});
        }
        else if (node.type == NodeType::OR) {
            // OR gate can be implemented as NOR followed by NOT
            int costWithNOR = NOR2_COST + NOT_COST + nodes[node.inputs[0]].cost + nodes[node.inputs[1]].cost;
            
            // Direct OR implementation
            int costWithOR = OR2_COST + nodes[node.inputs[0]].cost + nodes[node.inputs[1]].cost;
            
            // OR can also be implemented using NAND gates (De Morgan's laws)
            // NOT(A) NAND NOT(B) = A OR B
            int costWithNAND = NOT_COST + NOT_COST + NAND2_COST + 
                             nodes[node.inputs[0]].cost + nodes[node.inputs[1]].cost;
            
            // Choose the minimum cost
            node.cost = std::min({costWithNOR, costWithOR, costWithNAND});
        }
        
        return node.cost;
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
    
    // Write the result to output file
    std::ofstream outFile("output.txt");
    if (!outFile.is_open()) {
        std::cerr << "Error: Unable to open output file" << std::endl;
        return 1;
    }
    
    outFile << minimalCost;
    outFile.close();
    
    return 0;
}