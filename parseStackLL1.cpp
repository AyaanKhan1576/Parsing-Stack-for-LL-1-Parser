#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <stack> 

#include "sourceCFG.h"

using namespace std;

void CFGProcessor::parseInputFile(const string& inputFilename) {
    ifstream inputFile(inputFilename);
    if (!inputFile.is_open()) {
        cerr << "Error opening input file: " << inputFilename << endl;
        if (outputFile.is_open()) {
             outputFile << "Error opening input file: " << inputFilename << endl;
        }
        return;
    }

    cout << "\n===== PARSING INPUT STRINGS =====\n" << endl;
    if (outputFile.is_open()) {
        outputFile << "\n===== PARSING INPUT STRINGS =====\n" << endl;
    }

    string line;
    int lineNumber = 0;
    int errorCount = 0; // Local variable declaration instead of class member

    while (getline(inputFile, line)) {
        lineNumber++;

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        cout << "Line " << lineNumber << ": " << line << endl;
        if (outputFile.is_open()) {
            outputFile << "Line " << lineNumber << ": " << line << endl;
        }

        if (!parseString(line, lineNumber)) {
            // Error occurred during parsing
            errorCount++;
        }

        cout << "-----------------------------\n" << endl;
        if (outputFile.is_open()) {
            outputFile << "-----------------------------\n" << endl;
        }
    }

    cout << "Parsing completed with " << errorCount << " error(s)." << endl;
    if (outputFile.is_open()) {
        outputFile << "Parsing completed with " << errorCount << " error(s)." << endl;
    }

    inputFile.close();
}

// Get the next token from the input string
string CFGProcessor::getNextToken(const string& input, int& position) {
    // Skip whitespace
    while (position < input.length() && isspace(input[position])) {
        position++;
    }

    // If we've reached the end of the string, return end marker
    if (position >= input.length()) {
        return "$";
    }

    // Handle special tokens
    char c = input[position];
    if (c == '(' || c == ')' || c == '{' || c == '}' || c == ';' ||
        c == '=' || c == '+' || c == '-' || c == '*' || c == '/') {
        position++;
        return string(1, c);
    }

    // Handle multi-character operators (==, !=, >=, <=)
    if ((c == '=' || c == '!' || c == '>' || c == '<') &&
        position + 1 < input.length() && input[position + 1] == '=') {
        string op = input.substr(position, 2);
        position += 2;
        return op;
    }

    // Handle > and < operators
    if (c == '>' || c == '<') {
        position++;
        return string(1, c);
    }

    // Handle identifiers and keywords
    if (isalpha(c) || c == '_') {
        int start = position;
        while (position < input.length() &&
               (isalnum(input[position]) || input[position] == '_')) {
            position++;
        }
        string token = input.substr(start, position - start);

        // Check if it's a keyword (assuming keywords are defined in your grammar/terminals)
        // Example keywords:
         if (grammar.terminals.count(token)) { // Check if the token is a terminal (keyword)
            return token;
         }
        // if (token == "int" || token == "if") {
        //     return token;
        // }
        return "id"; // Return general identifier token if not a keyword terminal
    }

    // Handle integer literals
    if (isdigit(c)) {
        int start = position;
        while (position < input.length() && isdigit(input[position])) {
            position++;
        }
        // Check if "int_lit" is a defined terminal in your grammar
        if (grammar.terminals.count("int_lit")) {
             return "int_lit"; // Return integer literal token
        }
        // Fallback or handle as needed if "int_lit" isn't a terminal
        return input.substr(start, position - start); // Or return the actual number string
    }

    // If we couldn't recognize the token, return the character as an unknown token
    position++;
    // Consider how your grammar handles unknown characters. Returning it might cause parsing errors.
    // You might want to report an error here or have a specific "error" token.
    cerr << "Warning: Unrecognized character '" << c << "'" << endl;
    return string(1, c);
}

// Display the current state of the stack
void CFGProcessor::displayStack(stack<string> s, const string& input, int position) {
    vector<string> stackContents;

    // Extract stack contents (which reverses the order)
    while (!s.empty()) {
        stackContents.push_back(s.top());
        s.pop();
    }

    // Print the stack contents in correct order (bottom to top)
    cout << "Stack: [ ";
     if (outputFile.is_open()) outputFile << "Stack: [ ";
    for (int i = stackContents.size() - 1; i >= 0; i--) {
        cout << stackContents[i] << " ";
         if (outputFile.is_open()) outputFile << stackContents[i] << " ";
    }
    cout << "]" << endl;
     if (outputFile.is_open()) outputFile << "]" << endl;

    // Print the remaining input
    cout << "Input: ";
    if (outputFile.is_open()) outputFile << "Input: ";

    string restOfInput = "";
    // Need to reconstruct the remaining *token stream*, not just substring
    int tempPos = position;
    string token;
    string remainingTokens = "";
    while( (token = getNextToken(input, tempPos)) != "$" ) {
        remainingTokens += token + " ";
        if (tempPos >= input.length()) break; // Safety break
    }
    remainingTokens += "$";


    if (remainingTokens == "$") {
        cout << "$";
        if (outputFile.is_open()) outputFile << "$";
    } else {
        cout << remainingTokens;
         if (outputFile.is_open()) outputFile << remainingTokens;
    }
    cout << endl;
     if (outputFile.is_open()) outputFile << endl;
}

// Parse a single input string
bool CFGProcessor::parseString(const string& input, int lineNumber) {
    // Initialize the stack with the start symbol and end marker
    stack<string> parseStack;
    parseStack.push("$");  // Bottom of stack
    parseStack.push(grammar.startSymbol);

    int position = 0;
    string currentToken = getNextToken(input, position);
    bool error = false;
    int errorRecoveryCounter = 0; // Add counter to prevent infinite loops
    const int MAX_ERROR_RECOVERY_ATTEMPTS = 10; // Maximum number of consecutive error recovery attempts

    cout << "Beginning parse of input string" << endl;
    if (outputFile.is_open()) outputFile << "Beginning parse of input string" << endl;

    // Display initial state
    cout << "Current token: " << currentToken << endl;
    if (outputFile.is_open()) outputFile << "Current token: " << currentToken << endl;
    displayStack(parseStack, input, position);


    // Continue until we've processed all input or encountered an error
    while (!parseStack.empty() && errorRecoveryCounter < MAX_ERROR_RECOVERY_ATTEMPTS) {
        string topStack = parseStack.top();

        // If the top of the stack matches the current input token (or $)
        if (topStack == currentToken) {
             // Handle stack pop and token advance for match
             if (topStack == "$") {
                 cout << "Parse successful!" << endl;
                 if (outputFile.is_open()) outputFile << "Parse successful!" << endl;
                 parseStack.pop(); // Pop $
                 return true; // Successful parse
             }

            cout << "Match and Pop: " << topStack << endl;
            if (outputFile.is_open()) outputFile << "Match and Pop: " << topStack << endl;
            parseStack.pop();
            currentToken = getNextToken(input, position);
            cout << "Next token: " << currentToken << endl;
            if (outputFile.is_open()) outputFile << "Next token: " << currentToken << endl;
            errorRecoveryCounter = 0; // Reset error counter on successful match
        }
        // If the top of stack is a terminal but doesn't match current token
        else if (isTerminal(topStack)) {
            cout << "Syntax Error (Line " << lineNumber << "): Mismatch. Expected '" << topStack << "', but found '" << currentToken << "'" << endl;
            if (outputFile.is_open()) outputFile << "Syntax Error (Line " << lineNumber << "): Mismatch. Expected '" << topStack << "', but found '" << currentToken << "'" << endl;
            error = true;

            // Error Recovery Strategy 1: Panic Mode - Pop from stack
            cout << "Error Recovery: Popping '" << topStack << "' from stack." << endl;
             if (outputFile.is_open()) outputFile << "Error Recovery: Popping '" << topStack << "' from stack." << endl;
            parseStack.pop();
            errorRecoveryCounter++;

            // Optional: Add Strategy 2: Skip input token until one matches stack top or follow set element
        }
        // If the top of stack is a non-terminal
        else if (isNonTerminal(topStack)) {
            // Look up the production in the parse table
            if (parseTable.count({topStack, currentToken})) {
                vector<string> production = parseTable[{topStack, currentToken}];
                parseStack.pop(); // Pop the non-terminal

                cout << "Apply production: " << topStack << " -> ";
                if (outputFile.is_open()) outputFile << "Apply production: " << topStack << " -> ";
                for (const auto& symbol : production) {
                    cout << symbol << " ";
                    if (outputFile.is_open()) outputFile << symbol << " ";
                }
                cout << endl;
                 if (outputFile.is_open()) outputFile << endl;

                // Push production in reverse order onto the stack, skip epsilon
                if (!(production.size() == 1 && production[0] == "epsilon")) {
                    for (int i = production.size() - 1; i >= 0; i--) {
                        parseStack.push(production[i]);
                    }
                } else {
                     cout << "(Applying epsilon)" << endl;
                     if (outputFile.is_open()) outputFile << "(Applying epsilon)" << endl;
                }
                errorRecoveryCounter = 0; // Reset error counter on successful production application
            } else {
                // Error: No rule in parse table
                cout << "Syntax Error (Line " << lineNumber << "): No production for [" << topStack << ", " << currentToken << "]" << endl;
                if (outputFile.is_open()) outputFile << "Syntax Error (Line " << lineNumber << "): No production for [" << topStack << ", " << currentToken << "]" << endl;
                error = true;

                // Error Recovery Strategy 1: Panic Mode - Skip input token
                cout << "Error Recovery: Skipping token '" << currentToken << "'" << endl;
                if (outputFile.is_open()) outputFile << "Error Recovery: Skipping token '" << currentToken << "'" << endl;
                currentToken = getNextToken(input, position);
                errorRecoveryCounter++;

                 // Optional: Add Strategy 2: Pop Non-Terminal from stack if current token is in Follow(NonTerminal)
                 // if (followSets[topStack].count(currentToken)) {
                 //      cout << "Error Recovery: Popping '" << topStack << "' as token '" << currentToken << "' is in its FOLLOW set." << endl;
                 //      parseStack.pop();
                 // }
            }
        } else {
             // Should not happen with a correct parser/grammar setup
             cout << "Internal Parser Error: Unexpected stack top '" << topStack << "'" << endl;
             if (outputFile.is_open()) outputFile << "Internal Parser Error: Unexpected stack top '" << topStack << "'" << endl;
             return false; // Critical error
        }

        // Display current stack and input after action
         if (!parseStack.empty()){ // Avoid display if stack became empty (e.g., on success)
             displayStack(parseStack, input, position);
         }
         if (parseStack.empty() && currentToken != "$") {
             cout << "Syntax Error (Line " << lineNumber << "): Stack empty but input remains: " << currentToken << endl;
             if (outputFile.is_open()) outputFile << "Syntax Error (Line " << lineNumber << "): Stack empty but input remains: " << currentToken << endl;
             error = true;
             break; // Stop parsing
         }
    } // end while loop

    if (errorRecoveryCounter >= MAX_ERROR_RECOVERY_ATTEMPTS) {
        cout << "Too many consecutive errors. Giving up on line " << lineNumber << "." << endl;
        if (outputFile.is_open()) outputFile << "Too many consecutive errors. Giving up on line " << lineNumber << "." << endl;
        return false;
    }

    // Final check: Should be $ on stack and $ in input if successful
    if (parseStack.empty() && currentToken == "$") {
        // This case is handled inside the loop now for immediate success reporting
         return true; // Should have returned true already
    } else if (!error) {
        // If no error was flagged but we exit the loop not in success state
         cout << "Parse Error (Line " << lineNumber << "): Parsing ended unexpectedly. Stack top: "
              << (parseStack.empty() ? "empty" : parseStack.top()) << ", Input token: " << currentToken << endl;
          if (outputFile.is_open()) outputFile << "Parse Error (Line " << lineNumber << "): Parsing ended unexpectedly. Stack top: "
              << (parseStack.empty() ? "empty" : parseStack.top()) << ", Input token: " << currentToken << endl;
         return false;
    }


    return !error; // Return true if no errors were ultimately flagged (or handled)
}

// --- Main Function (Modified for Assignment 3) ---

int main(int argc, char* argv[]) {
    if (argc != 4) {
        // Updated usage message for Assignment 3
        cerr << "Usage: " << argv[0] << " grammar.txt input.txt output.txt" << endl;
        return 1;
    }

    // Constructor now needs output filename (as defined in sourceCFG.cpp)
    CFGProcessor processor(argv[1], argv[3]);

    // Perform grammar processing (Left Factoring, Left Recursion, FIRST/FOLLOW, Parse Table)
    // displayResults calls the necessary methods from the included sourceCFG.cpp
    processor.displayResults();

    // Parse the input strings using the new method for Assignment 3
    processor.parseInputFile(argv[2]);

    cout << "\nProcessing complete. Results are in " << argv[3] << endl;
     if(processor.outputFile.is_open()) { // Check if file is open before writing final message
         processor.outputFile << "\nProcessing complete." << endl;
     }

    // Destructor will close the output file automatically

    return 0;
}