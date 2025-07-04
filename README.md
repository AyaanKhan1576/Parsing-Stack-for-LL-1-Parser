﻿# LL(1) Parser

## Contributors
- Ayaan Khan
- Minahil Ali

## Overview
The LL(1) Parser is a fully-featured top-down parser designed to process context-free grammars (CFGs) and input strings. It implements the LL(1) parsing algorithm, which is a predictive parsing technique that uses a single lookahead token to make parsing decisions. The application provides detailed reports, including FIRST/FOLLOW sets, LL(1) parsing tables, and step-by-step parsing traces.

## Features
1. **FIRST/FOLLOW Sets Generation**: Computes the FIRST and FOLLOW sets for the given grammar.
2. **LL(1) Parsing Table**: Constructs and pretty-prints the LL(1) parsing table.
3. **Step-by-Step Parsing Trace**: Displays the parsing process in a `Stack | Input | Action` table format.
4. **Syntax Error Detection**: Identifies syntax errors and provides detailed feedback.
5. **Console and File Output**: Streams all reports to both the console and a user-specified output file.

## Application Workflow
The application processes three files:
1. **Grammar File (`grammar.txt`)**: Contains the context-free grammar, with one production per line. Productions use `->` to denote derivations.
2. **Input File (`input.txt`)**: Contains space-separated input strings, with each non-empty line parsed independently.
3. **Output File (`output.txt`)**: Stores all console outputs for easy submission.

### Example Usage
```bash
$ ./app grammar.txt input.txt output.txt
```
The console and `output.txt` will display detailed parsing results, including the LL(1) parsing table and parsing traces for each input string.

---

## File Details

### `sourceCFG.cpp`
This file handles the processing of the context-free grammar (CFG). Its main functionalities include:
1. **Grammar Parsing**: Reads and parses the grammar file to extract productions.
2. **FIRST Set Computation**: Calculates the FIRST set for each non-terminal in the grammar.
3. **FOLLOW Set Computation**: Computes the FOLLOW set for each non-terminal based on the grammar.
4. **LL(1) Parsing Table Construction**: Builds the LL(1) parsing table using the FIRST and FOLLOW sets.
5. **Error Handling**: Detects and reports issues in the grammar, such as left recursion or conflicts.

### Key Functions
- `computeFirstSet()`: Calculates the FIRST set for all non-terminals.
- `computeFollowSet()`: Computes the FOLLOW set for all non-terminals.
- `buildParsingTable()`: Constructs the LL(1) parsing table.
- `displayResults()`: Outputs the computed FIRST/FOLLOW sets and parsing table.

---

### `parseStack.cpp`
This file implements the LL(1) parsing algorithm and handles the parsing of input strings. Its main functionalities include:
1. **Parsing Input Strings**: Reads the input file and parses each line using the LL(1) parsing table.
2. **Stack Operations**: Manages the parsing stack during the parsing process.
3. **Error Detection**: Identifies syntax errors and provides detailed feedback.
4. **Pretty-Printing**: Formats and displays the parsing trace in a tabular format.

### Key Functions
- `parseInputFile()`: Reads the input file and parses each line.
- `parseString()`: Implements the LL(1) parsing algorithm for a single input string.
- `displayStack()`: Pretty-prints the current state of the stack, input, and action.
- `getNextToken()`: Tokenizes the input string for parsing.

---

## Application Structure
The application is divided into two main components:
1. **Grammar Processing (`sourceCFG.cpp`)**: Handles the grammar file and constructs the parsing table.
2. **Input Parsing (`parseStack.cpp`)**: Uses the parsing table to process input strings and generate parsing traces.

### Workflow
1. **Grammar File Processing**:
   - Parse the grammar file.
   - Compute FIRST and FOLLOW sets.
   - Build the LL(1) parsing table.
2. **Input File Parsing**:
   - Read input strings.
   - Parse each string using the LL(1) parsing table.
   - Generate parsing traces and error reports.

---

## Building the Application
### One-Liner (POSIX Shell)
```bash
$ g++ -o app .\parseStack.cpp .\sourceCFG.cpp
```
This command compiles the application into a single executable named `app`.

---

## Running the Application
### Command
```bash
$ ./app grammar.txt input.txt output.txt
```
### Input Files
- **`grammar.txt`**: Contains the context-free grammar.
- **`input.txt`**: Contains input strings to be parsed.
- **`output.txt`**: Stores the parsing results.

### Output
The application generates detailed reports, including:
1. LL(1) Parsing Table.
2. Parsing traces for each input string.
3. Syntax error feedback.

The console (and `output.txt`) will now show:

```
===== LL(1) Parsing Table =====
…
===== PARSING INPUT STRINGS =====
Code  : int x ;
+----------------------------------------+----------------------------------------+------------------------------+
| STACK                                  | INPUT                                  | ACTION                       |
+----------------------------------------+----------------------------------------+------------------------------+
| S $                                    | int id ; $                             | Initial state                |
| …                                      | …                                      | …                            |
+----------------------------------------+----------------------------------------+------------------------------+
Result: Line 1 parsed successfully.
```

