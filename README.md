# EEL4837 Excursion 2 Technology Mapping
# Group Friendship is Magic: 
- Allison Freeman
- Emily Wang 
- Isabella Cratem

This is a C++ tool we built to figure out the cheapest way to implement a logic circuit using just NAND and NOT gates. It reads in a Boolean netlist and finds the lowest-cost setup possible.
This tool reads a circuit made up of logic gates and tells you the cheapest way to build it using only NAND and NOT gates. It’s a logic optimization project — part school, part puzzle.

## What We Were Trying to Do
1. Read a logic circuit from a text file.
2. Break it down into pieces.
3. Figure out the minimum cost to build it.
4. Write that cost to a file.

We take a netlist (a list of logic gate connections), and go gate-by-gate to figure out how to implement each one using NAND and NOT gates. If there’s a cheaper way to represent a gate using NANDs and NOTs, we use that.

## How It Works
- Written in C++
- Reads logic gates like AND, OR, NOT, NAND2, NOR2, AOI21, AOI22
- Uses recursive functions and memoization to avoid recalculating stuff
- Always starts from the output and works backward
- Makes sure we don't reuse gates incorrectly or collapse them when we shouldn't

## Features
- Reads Boolean netlists line by line
- Tracks every node and its type (AND, OR, etc.)
- Figures out the cost of every gate and its children
- Compares multiple ways to build each gate using NAND/NOT and picks the cheapest
- Doesn’t accidentally merge gates that are reused somewhere else

## Install & Run
- Any C++17 compiler (g++, clang++)
- make sure `input.txt` exists. The result will be saved in `output.txt`.

## File Layout
Technology_Mapping/
├── input.txt        # Netlist file
├── output.txt       # Where the result goes
├── final_tm.cpp         # Our main code
└── README.md        # You're reading it!

## How to Use It
Example netlist format:
input.txt
a INPUT
b INPUT
c INPUT
t1 = AND a b
t2 = NOT t1
F OUTPUT
t2

Run the tool:
output.txt will generated

You’ll see the minimum cost printed out.

## Cost Rules
Gate     | Cost 
NOT      | 2    
NAND2    | 3    
AND2     | 4    
OR2      | 4    
NOR2     | 6    
AOI21    | 7    
AOI22    | 7    
We check if a gate like AND can be made with NAND+NOT (3+2=5) and pick whichever is cheaper.

## How We Built It

Here’s a breakdown of what each part of the code does:
# readNetlist()
This reads input.txt line-by-line. For every line, it figures out if the line is an input, output, or a gate (AND, OR, NOT, etc.). It creates a Node for each one and stores them in a map so we can look them up by name later. It also keeps track of the output node, which is where we start the evaluation.

# calculateMinimalCost()
This resets all the visited and cost flags for each node and then kicks off evaluation starting from the output node. This is the main driver function that returns the final result.

# eval()

This is the core logic. It takes a node name, looks it up, and calculates the cost to implement it by:
Recursively evaluating the cost of all its input nodes
Based on the gate type, calculating all valid NAND/NOT-based implementations
Picking the lowest cost option
Memoizing the result so we don’t do extra work
We do not change the gate types or structure here — we just look at each gate, think “what are the possible legal ways to build this using NAND/NOT?”, and pick the cheapest.

## Test 8 — What Went Wrong
### The Problem
In Test 8, we kept getting a result of **22**, but the correct answer was **29**. That’s a big difference.

### Why It Happened
Our original code saving cost by folding gates when it saw common patterns. For example, it would spot something like:

t1 = AND b c
t2 = NOT t1

will think just a nand gate, so it would combine t1 and t2 into a single NAND2 and give it a cost of 3 instead of 4 + 2 = 6.
That works fine if t1 is only used once. But in Test 8, t1 is used more than once in different places. By folding it into a NAND too early, we broke its other usage. The memoized cost of t1 got reused elsewhere, and now all those places think the cost is 3 — even though it should’ve stayed as a full AND gate.

Basically, our code was changing the meaning of the netlist without realizing it. We were treating t2 = NOT t1 as if it replaced t1, but in circuits, t1 can be used in multiple places. So the moment we reused that cheaper cost, everything else downstream was wrong.

### What We Tried
Tracked how many times each node was used
Only folded gates when they were used once
Checked if a NOT was wrapping an AND, OR, or NAND and replaced it

We tried a lot:
We tracked how many times each node was used to prevent folding shared gates.
We only folded things like NOT(AND(...)) into NAND when the inner gate was used once.
We added specific logic to catch and replace known patterns like NOT(OR(...)), NOT(NAND(...)) too.

And honestly, it worked on a lot of simpler cases. But Test 8? Still gave us 22. That told us something deeper was off — even with usage counts and pattern guards, the cost was leaking between shared nodes. Either a node got folded in one place and reused incorrectly in another, or the cost caching didn’t respect context. No matter how we patched it, we couldn’t get a fully reliable solution that handled all the test cases, especially Test 8

## Credits
- Allison Freeman
- Emily Wang 
- Isabella Cratem