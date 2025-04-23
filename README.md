# EEL4837 Excursion 2 Technology Mapping
# Group Friendship is Magic: 
- Allison Freeman
- Emily Wang 
- Isabella Cratem

This is a C++ tool built to figure out the cheapest way to implement a logic circuit. It reads in a Boolean netlist and finds the lowest-cost setup possible.

## Steps
1. Read a input text file
2. Create a tree using Nodes
3. Calculate the minimum cost using recursion and memoization
4. Write that cost to an output text file

## Install & Run
- Any C++17 compiler
- make sure input.txt exists in the same working directory or is explicity mentioned in main()
- The result will be saved in output.txt. The file will be created if not already there

## File Layout
Technology_Mapping -
input.txt      
output.txt     
final_tm.cpp     
README.md      

## Breakdown of Code

Here is a list and short description of the major functions used in the project.

# readNetlist()
This reads input.txt line-by-line. For every line, it figures out if the line is an input, output, or a gate (AND, OR, NOT, etc.). It creates a Node for each one and stores them in a map so we can look them up by name later. It also keeps track of the output node, which is where we start the evaluation.

# minCost()
Initializes the Nodes as not visited and the cost as -1. Calls the function, patterns(), which will recursively determine the lowest cost from the existing Node tree.

# patternz()
This is the core logic. It takes a node name, looks it up, and calculates the cost to implement it by:
- Recursively evaluating the cost of all its input nodes
- Based on the gate type, calculating all valid NAND/NOT-based implementations
- Picking the lowest cost option
- Memoizing the result so we don’t do extra work :)

## Test 8 — What Went Wrong

In Test Case 8, we kept getting a result of **34**, but the correct answer was **29**.

### Why
The input file of test case 8 defines the output node by T = G AND M, which is different than how the other input files define the output. In our pattern recognition funtion, it fails to optimize one piece of the logic. The code is looking for specific gate combinations that don't align with a part of this particular circuit's structure.

We **think** the pattern between AND(AND, NOT(OR)) is not being recognized and optimized correctly.

### What We Tried
We added multiple debug statements and even printed out the node tree that is created, but it was difficult to find a solution that allowed for the pattern of test case 8 to be found as well as the other test cases. 

## Credits
- Allison Freeman
- Emily Wang 
- Isabella Cratem
