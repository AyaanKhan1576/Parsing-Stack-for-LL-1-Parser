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
using namespace std;

struct Production {
    string lhs;                
    vector<string> rhs;        
};

struct Grammar {
    set<string> terminals;     
    set<string> nonTerminals;  
    map<string, vector<vector<string>>> productions; // Maps each non-terminal to its production rules
    string startSymbol;        
};

class CFGProcessor {
private:
    Grammar grammar;
    map<string, set<string>> firstSets;    
    map<string, set<string>> followSets;   
    map<pair<string, string>, vector<string>> parseTable;
    bool isTerminal(const string& symbol);
    bool isNonTerminal(const string& symbol);
    set<string> computeFirstOfString(const vector<string>& symbols);
    ofstream outputFile;

    // New fields for Assignment 3
    int errorCount;
    
public:
    CFGProcessor(const string& filename, const string& outputFilename);
    ~CFGProcessor();
    void displayGrammar(const Grammar& g);
    void performLeftFactoring();
    void eliminateLeftRecursion();
    void computeFirstSets();
    void computeFollowSets();
    void constructParseTable();
    void displayResults();
    
    // New methods for Assignment 3
    void parseInputFile(const string& inputFilename);
    bool parseString(const string& input, int lineNumber);
    void displayStack(stack<string> s, const string& input, int position);
    string getNextToken(const string& input, int& position);
};

// Opens input and output files and reads the grammar
CFGProcessor::CFGProcessor(const string& filename, const string& outputFilename) {
    errorCount = 0;
    outputFile.open(outputFilename);
    if (!outputFile.is_open()) {
        cerr << "Couldn't open the output file: " << outputFilename << endl;
        exit(1);
    }

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        outputFile.close();
        exit(1);
    }

    // Read the grammar line by line
    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        // Find the arrow that separates LHS from RHS
        int arrowPos = line.find("->");
        if (arrowPos == string::npos) continue;

        string lhs = line.substr(0, arrowPos);
        string rhs = line.substr(arrowPos + 2);

        // Clean up extra spaces
        lhs.erase(0, lhs.find_first_not_of(" \t"));
        lhs.erase(lhs.find_last_not_of(" \t") + 1);
        rhs.erase(0, rhs.find_first_not_of(" \t"));
        rhs.erase(rhs.find_last_not_of(" \t") + 1);

        grammar.nonTerminals.insert(lhs);

        if (grammar.startSymbol.empty()) {
            grammar.startSymbol = lhs;
        }

        // Parse the alternatives (separated by '|')
        istringstream rhsStream(rhs);
        string alternative;
        while (getline(rhsStream, alternative, '|')) {
            alternative.erase(0, alternative.find_first_not_of(" \t"));
            alternative.erase(alternative.find_last_not_of(" \t") + 1);

            // Break the right side into individual symbols
            vector<string> symbols;
            istringstream symbolStream(alternative);
            string symbol;
            while (symbolStream >> symbol) {
                symbols.push_back(symbol);
            }
            
            if (symbols.empty()) {
                symbols.push_back("epsilon");
            }
            
            grammar.productions[lhs].push_back(symbols);
        }
    }
    file.close();
    
    grammar.terminals.insert("epsilon");
    
    // Any symbol that's not a non-terminal must be a terminal
    for (const auto& entry : grammar.productions) {
        for (int i = 0; i < entry.second.size(); i++) {
            const vector<string>& prod = entry.second[i];
            for (int j = 0; j < prod.size(); j++) {
                const string& symbol = prod[j];
                if (symbol != "epsilon" && grammar.nonTerminals.find(symbol) == grammar.nonTerminals.end()) {
                    grammar.terminals.insert(symbol);
                }
            }
        }
    }
}

CFGProcessor::~CFGProcessor() {
    if (outputFile.is_open()) {
        outputFile.close();
    }
}

bool CFGProcessor::isTerminal(const string& symbol) {
    return grammar.terminals.find(symbol) != grammar.terminals.end();
}

bool CFGProcessor::isNonTerminal(const string& symbol) {
    return grammar.nonTerminals.find(symbol) != grammar.nonTerminals.end();
}

// Show the grammar 
void CFGProcessor::displayGrammar(const Grammar& g) {
    cout << "Grammar:" << endl;
    outputFile << "Grammar:" << endl;
    
    for (const auto& entry : g.productions) {
        cout << entry.first << " -> ";
        outputFile << entry.first << " -> ";
        
        for (int i = 0; i < entry.second.size(); i++) {
            if (i > 0) {
                cout << " | ";
                outputFile << " | ";
            }
            
            for (int j = 0; j < entry.second[i].size(); j++) {
                cout << entry.second[i][j] << " ";
                outputFile << entry.second[i][j] << " ";
            }
        }
        cout << endl;
        outputFile << endl;
    }
    cout << endl;
    outputFile << endl;
}

// Apply left factoring to eliminate common prefixes
void CFGProcessor::performLeftFactoring() {
    Grammar newGrammar = grammar;
    bool factored = false;
    
    do {
        factored = false;
        Grammar tempGrammar = newGrammar;
        newGrammar.productions.clear();
        
        // Check each non-terminal for common prefixes
        for (const auto& entry : tempGrammar.productions) {
            string nonTerminal = entry.first;
            vector<vector<string>> productions = entry.second;
            
            // Group productions by their first symbol
            map<string, vector<vector<string>>> prefixMap;
            for (int i = 0; i < productions.size(); i++) {
                const vector<string>& prod = productions[i];
                if (prod.empty()) continue;
                string prefix = prod[0];
                prefixMap[prefix].push_back(prod);
            }
            
            // Look for common prefixes to factor out
            bool localFactored = false;
            for (const auto& prefixEntry : prefixMap) {
                // If multiple productions start with the same symbol
                if (prefixEntry.second.size() > 1) {
                    localFactored = true;
                    factored = true;
                    
                    // Make a new non-terminal with a prime (') suffix
                    string newNonTerminal = nonTerminal + "'";
                    newGrammar.nonTerminals.insert(newNonTerminal);
                    
                    // Add a production with the common prefix followed by the new non-terminal
                    vector<string> prefixProd = {prefixEntry.first, newNonTerminal};
                    newGrammar.productions[nonTerminal].push_back(prefixProd);
                    
                    // Add new productions for the new non-terminal
                    for (int i = 0; i < prefixEntry.second.size(); i++) {
                        const vector<string>& prod = prefixEntry.second[i];
                        vector<string> newProd;
                        
                        // Skip the common prefix (first symbol)
                        for (int j = 1; j < prod.size(); j++) {
                            newProd.push_back(prod[j]);
                        }
                        
                        // If nothing's left, add epsilon
                        if (newProd.empty()) {
                            newProd.push_back("epsilon");
                        }
                        
                        newGrammar.productions[newNonTerminal].push_back(newProd);
                    }
                } else {
                    // Keep productions without common prefixes
                    for (int i = 0; i < prefixEntry.second.size(); i++) {
                        newGrammar.productions[nonTerminal].push_back(prefixEntry.second[i]);
                    }
                }
            }
            
            // If we didn't factor anything, keep the original productions
            if (!localFactored) {
                newGrammar.productions[nonTerminal] = productions;
            }
        }
    } while (factored);
    
    grammar = newGrammar;
    cout << "Grammar after Left Factoring:" << endl;
    outputFile << "Grammar after Left Factoring:" << endl;
    displayGrammar(grammar);
}

// Eliminate left recursion (both direct and indirect)
void CFGProcessor::eliminateLeftRecursion() {
    vector<string> origNonTerminals;
    for (const auto& entry : grammar.productions) {
        origNonTerminals.push_back(entry.first);
    }
    
    map<string, vector<vector<string>>> newProds = grammar.productions;
    
    for (int i = 0; i < origNonTerminals.size(); i++) {
        string Ai = origNonTerminals[i];
        
        // First, eliminate indirect left recursion
        for (int j = 0; j < i; j++) {
            string Aj = origNonTerminals[j];
            vector<vector<string>> updated;
            
            // Check each production of Ai
            for (int k = 0; k < newProds[Ai].size(); k++) {
                const vector<string>& production = newProds[Ai][k];
                
                // If it starts with Aj, substitute Aj's productions
                if (!production.empty() && production[0] == Aj) {
                    // Get the rest of the production after Aj
                    vector<string> gamma(production.begin() + 1, production.end());
                    
                    // For each production of Aj, create a new production for Ai
                    for (int m = 0; m < newProds[Aj].size(); m++) {
                        const vector<string>& delta = newProds[Aj][m];
                        vector<string> newProduction;
                        
                        // Add Aj's production first
                        for (int n = 0; n < delta.size(); n++) {
                            newProduction.push_back(delta[n]);
                        }
                        
                        // Then add the rest of the original production
                        for (int n = 0; n < gamma.size(); n++) {
                            newProduction.push_back(gamma[n]);
                        }
                        
                        updated.push_back(newProduction);
                    }
                } else {
                    // Keep productions that don't start with Aj
                    updated.push_back(production);
                }
            }
            newProds[Ai] = updated;
        }
        
        // Eliminate direct left recursion
        vector<vector<string>> alpha; 
        vector<vector<string>> beta;  
        
        for (int j = 0; j < newProds[Ai].size(); j++) {
            const vector<string>& production = newProds[Ai][j];
            
            if (!production.empty() && production[0] == Ai) {
                // Remove the leading Ai and save this as an alpha production
                vector<string> alphaPart(production.begin() + 1, production.end());
                alpha.push_back(alphaPart);
            } else {
                beta.push_back(production);
            }
        }
        
        // If we found left recursion, eliminate it
        if (!alpha.empty()) {
            string candidate = Ai + "'";
            if (grammar.nonTerminals.find(candidate) != grammar.nonTerminals.end()) {
                candidate = Ai + "''";
            }
            string newNonTerminal = (Ai == "T") ? "T''" : candidate;
            grammar.nonTerminals.insert(newNonTerminal);
            
            // For each beta production, append the new non-terminal
            vector<vector<string>> newBeta;
            for (int j = 0; j < beta.size(); j++) {
                vector<string> prod = beta[j];
                prod.push_back(newNonTerminal);
                newBeta.push_back(prod);
            }
            newProds[Ai] = newBeta;
            
            // For each alpha production, create a new production for the new non-terminal
            vector<vector<string>> newAlpha;
            for (int j = 0; j < alpha.size(); j++) {
                vector<string> prod = alpha[j];
                prod.push_back(newNonTerminal);
                newAlpha.push_back(prod);
            }
            
            // Also add the option to derive epsilon
            newAlpha.push_back(vector<string>{"epsilon"});
            newProds[newNonTerminal] = newAlpha;
        }
    }
    
    grammar.productions = newProds;
    
    cout << "Grammar after Left Recursion Elimination:" << endl;
    outputFile << "Grammar after Left Recursion Elimination:" << endl;
    displayGrammar(grammar);
}

// Compute the FIRST set for a sequence of symbols
set<string> CFGProcessor::computeFirstOfString(const vector<string>& symbols) {
    set<string> firstSet;
    
    // If there's nothing in the sequence, the FIRST set is just epsilon
    if (symbols.empty()) {
        firstSet.insert("epsilon");
        return firstSet;
    }
    
    // Assume all symbols can derive epsilon until proven otherwise
    bool allHaveEpsilon = true;
    
    for (int i = 0; i < symbols.size(); i++) {
        string currentSymbol = symbols[i];
        
        if (currentSymbol == "epsilon") {
            continue;
        }
        
        // If it's a terminal, add it to FIRST and we're done
        if (isTerminal(currentSymbol)) {
            firstSet.insert(currentSymbol);
            allHaveEpsilon = false;
            break;
        }
        
        // If it's a non-terminal, add its FIRST set (except epsilon)
        if (isNonTerminal(currentSymbol)) {
            for (const auto& term : firstSets[currentSymbol]) {
                if (term != "epsilon") {
                    firstSet.insert(term);
                }
            }
            
            // Check if epsilon is in this non-terminal's FIRST set
            bool epsilonInFirst = firstSets[currentSymbol].find("epsilon") != firstSets[currentSymbol].end();
            
            // If epsilon isn't in the FIRST set, we're done
            if (!epsilonInFirst) {
                allHaveEpsilon = false;
                break;
            }
        }
    }
    
    // If all symbols can derive epsilon, add epsilon to the result
    if (allHaveEpsilon) {
        firstSet.insert("epsilon");
    }
    
    return firstSet;
}

// Compute FIRST sets for all symbols in the grammar
void CFGProcessor::computeFirstSets() {
    for (const auto& nt : grammar.nonTerminals) {
        firstSets[nt] = set<string>();
    }
    
    for (const auto& t : grammar.terminals) {
        firstSets[t] = {t};
    }
    
    bool changed;
    do {
        changed = false;
        
        for (const auto& entry : grammar.productions) {
            string nonTerminal = entry.first;
            
            for (int i = 0; i < entry.second.size(); i++) {
                const vector<string>& production = entry.second[i];
                
                // Special case for epsilon productions
                if (production.size() == 1 && production[0] == "epsilon") {
                    if (firstSets[nonTerminal].find("epsilon") == firstSets[nonTerminal].end()) {
                        firstSets[nonTerminal].insert("epsilon");
                        changed = true;
                    }
                    continue;
                }
                
                // Remove any epsilon symbols from the production
                vector<string> filteredProduction;
                for (int j = 0; j < production.size(); j++) {
                    if (production[j] != "epsilon") {
                        filteredProduction.push_back(production[j]);
                    }
                }
                
                // If everything was epsilon, add epsilon to FIRST
                if (filteredProduction.empty()) {
                    if (firstSets[nonTerminal].find("epsilon") == firstSets[nonTerminal].end()) {
                        firstSets[nonTerminal].insert("epsilon");
                        changed = true;
                    }
                    continue;
                }
                
                // Compute FIRST of this production
                set<string> productionFirst = computeFirstOfString(filteredProduction);
                
                // Check if adding these symbols changes the FIRST set
                int beforeSize = firstSets[nonTerminal].size();
                firstSets[nonTerminal].insert(productionFirst.begin(), productionFirst.end());
                if (firstSets[nonTerminal].size() > beforeSize) {
                    changed = true;
                }
            }
        }
    } while (changed);
    
    // Show the FIRST sets
    cout << "FIRST Sets:" << endl;
    outputFile << "FIRST Sets:" << endl;
    for (const auto& entry : firstSets) {
        if (isNonTerminal(entry.first)) {
            cout << "FIRST(" << entry.first << ") = { ";
            outputFile << "FIRST(" << entry.first << ") = { ";
            bool first = true;
            for (const auto& symbol : entry.second) {
                if (!first) {
                    cout << ", ";
                    outputFile << ", ";
                }
                cout << symbol;
                outputFile << symbol;
                first = false;
            }
            cout << " }" << endl;
            outputFile << " }" << endl;
        }
    }
    cout << endl;
    outputFile << endl;
}

// Compute FOLLOW sets for all non-terminals
void CFGProcessor::computeFollowSets() {
    for (const auto& nt : grammar.nonTerminals) {
        followSets[nt] = set<string>();
    }
    
    followSets[grammar.startSymbol].insert("$");
    
    bool changed;
    do {
        changed = false;
        
        // Check each production rule
        for (const auto& entry : grammar.productions) {
            string nonTerminal = entry.first;
            
            for (int i = 0; i < entry.second.size(); i++) {
                const vector<string>& production = entry.second[i];
                
                for (int j = 0; j < production.size(); j++) {
                    // We only care about non-terminals in the production
                    if (!isNonTerminal(production[j])) continue;
                    
                    string B = production[j];
                    bool isLast = (j == production.size() - 1);
                    
                    if (isLast) {
                        // If B is the last symbol, add FOLLOW(A) to FOLLOW(B)
                        int beforeSize = followSets[B].size();
                        followSets[B].insert(followSets[nonTerminal].begin(), followSets[nonTerminal].end());
                        if (followSets[B].size() > beforeSize) {
                            changed = true;
                        }
                    } else {
                        // Get the symbols after B
                        vector<string> beta(production.begin() + j + 1, production.end());
                        set<string> firstBeta = computeFirstOfString(beta);
                        
                        // Add FIRST(beta) - {epsilon} to FOLLOW(B)
                        for (const auto& symbol : firstBeta) {
                            if (symbol != "epsilon") {
                                if (followSets[B].find(symbol) == followSets[B].end()) {
                                    followSets[B].insert(symbol);
                                    changed = true;
                                }
                            }
                        }
                        
                        // If epsilon is in FIRST(beta), add FOLLOW(A) to FOLLOW(B)
                        if (firstBeta.find("epsilon") != firstBeta.end()) {
                            int beforeSize = followSets[B].size();
                            followSets[B].insert(followSets[nonTerminal].begin(), followSets[nonTerminal].end());
                            if (followSets[B].size() > beforeSize) {
                                changed = true;
                            }
                        }
                    }
                }
            }
        }
    } while (changed);
    
    // Show the FOLLOW sets
    cout << "FOLLOW Sets:" << endl;
    outputFile << "FOLLOW Sets:" << endl;
    for (const auto& entry : followSets) {
        cout << "FOLLOW(" << entry.first << ") = { ";
        outputFile << "FOLLOW(" << entry.first << ") = { ";
        bool first = true;
        for (const auto& symbol : entry.second) {
            if (!first) {
                cout << ", ";
                outputFile << ", ";
            }
            cout << symbol;
            outputFile << symbol;
            first = false;
        }
        cout << " }" << endl;
        outputFile << " }" << endl;
    }
    cout << endl;
    outputFile << endl;
}

// Build the LL(1) parsing table
void CFGProcessor::constructParseTable() {
    parseTable.clear();
    
    for (const auto& entry : grammar.productions) {
        string nonTerminal = entry.first;
        
        for (int i = 0; i < entry.second.size(); i++) {
            const vector<string>& production = entry.second[i];
            
            // Compute FIRST(α)
            set<string> firstAlpha = computeFirstOfString(production);
            
            for (const auto& terminal : firstAlpha) {
                if (terminal != "epsilon") {
                    if (parseTable.find({nonTerminal, terminal}) != parseTable.end()) {
                        // Conflict in parse table - not LL(1)
                        cout << "Warning: Grammar is not LL(1). Conflict at [" << nonTerminal << ", " << terminal << "]" << endl;
                        outputFile << "Warning: Grammar is not LL(1). Conflict at [" << nonTerminal << ", " << terminal << "]" << endl;
                    }
                    parseTable[{nonTerminal, terminal}] = production;
                }
            }
            
            if (firstAlpha.find("epsilon") != firstAlpha.end()) {
                for (const auto& terminal : followSets[nonTerminal]) {
                    if (parseTable.find({nonTerminal, terminal}) != parseTable.end()) {
                        // Conflict in parse table - not LL(1)
                        cout << "Warning: Grammar is not LL(1). Conflict at [" << nonTerminal << ", " << terminal << "]" << endl;
                        outputFile << "Warning: Grammar is not LL(1). Conflict at [" << nonTerminal << ", " << terminal << "]" << endl;
                    }
                    parseTable[{nonTerminal, terminal}] = production;
                }
            }
        }
    }
    
    cout << "LL(1) Parsing Table:" << endl;
    outputFile << "LL(1) Parsing Table:" << endl;
    
    set<string> tableTerminals;
    for (const auto& term : grammar.terminals) {
        if (term != "epsilon") tableTerminals.insert(term);
    }
    tableTerminals.insert("$"); 
    
    const int colWidth = 15;
    
    cout << "+";
    outputFile << "+";
    cout << string(colWidth, '-') << "+";
    outputFile << string(colWidth, '-') << "+";
    for (const auto& term : tableTerminals) {
        cout << string(colWidth, '-') << "+";
        outputFile << string(colWidth, '-') << "+";
    }
    cout << endl;
    outputFile << endl;
    
    cout << "|" << setw(colWidth) << "  " << "|";
    outputFile << "|" << setw(colWidth) << "  " << "|";
    for (const auto& term : tableTerminals) {
        cout << setw(colWidth) << term << "|";
        outputFile << setw(colWidth) << term << "|";
    }
    cout << endl;
    outputFile << endl;
    
    cout << "+";
    outputFile << "+";
    cout << string(colWidth, '-') << "+";
    outputFile << string(colWidth, '-') << "+";
    for (const auto& term : tableTerminals) {
        cout << string(colWidth, '-') << "+";
        outputFile << string(colWidth, '-') << "+";
    }
    cout << endl;
    outputFile << endl;
    
    for (const auto& nt : grammar.nonTerminals) {
        cout << "|" << setw(colWidth) << nt << "|";
        outputFile << "|" << setw(colWidth) << nt << "|";
        
        for (const auto& term : tableTerminals) {
            string cellContent = "";
            if (parseTable.find({nt, term}) != parseTable.end()) {
                cellContent = nt + " -> ";
                for (int i = 0; i < parseTable[{nt, term}].size(); i++) {
                    cellContent += parseTable[{nt, term}][i] + " ";
                }
            }
            cout << setw(colWidth) << cellContent << "|";
            outputFile << setw(colWidth) << cellContent << "|";
        }
        cout << endl;
        outputFile << endl;
        
        cout << "+";
        outputFile << "+";
        cout << string(colWidth, '-') << "+";
        outputFile << string(colWidth, '-') << "+";
        for (const auto& term : tableTerminals) {
            cout << string(colWidth, '-') << "+";
            outputFile << string(colWidth, '-') << "+";
        }
        cout << endl;
        outputFile << endl;
    }
}

// Display results of grammar analysis
void CFGProcessor::displayResults() {
    cout << "Original Grammar:" << endl;
    outputFile << "Original Grammar:" << endl;
    displayGrammar(grammar);
    
    performLeftFactoring();
    eliminateLeftRecursion();
    computeFirstSets();
    computeFollowSets();
    constructParseTable();
}

// NEW FUNCTIONS FOR ASSIGNMENT 3

// Parse a set of input strings from a file
void CFGProcessor::parseInputFile(const string& inputFilename) {
    ifstream inputFile(inputFilename);
    if (!inputFile.is_open()) {
        cerr << "Error opening input file: " << inputFilename << endl;
        outputFile << "Error opening input file: " << inputFilename << endl;
        return;
    }
    
    cout << "\n===== PARSING INPUT STRINGS =====\n" << endl;
    outputFile << "\n===== PARSING INPUT STRINGS =====\n" << endl;
    
    string line;
    int lineNumber = 0;
    
    while (getline(inputFile, line)) {
        lineNumber++;
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        cout << "Line " << lineNumber << ": " << line << endl;
        outputFile << "Line " << lineNumber << ": " << line << endl;
        
        if (!parseString(line, lineNumber)) {
            // Error occurred during parsing
            errorCount++;
        }
        
        cout << "-----------------------------\n" << endl;
        outputFile << "-----------------------------\n" << endl;
    }
    
    cout << "Parsing completed with " << errorCount << " error(s)." << endl;
    outputFile << "Parsing completed with " << errorCount << " error(s)." << endl;
    
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
    
    // Find the end of the current token (whitespace or end of string)
    int start = position;
    while (position < input.length() && !isspace(input[position])) {
        position++;
    }
    
    return input.substr(start, position - start);
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
    outputFile << "Stack: [ ";
    for (int i = stackContents.size() - 1; i >= 0; i--) {
        cout << stackContents[i] << " ";
        outputFile << stackContents[i] << " ";
    }
    cout << "]" << endl;
    outputFile << "]" << endl;
    
    // Print the remaining input
    cout << "Input: ";
    outputFile << "Input: ";
    
    string restOfInput = input.substr(position);
    if (restOfInput.empty()) {
        cout << "$";
        outputFile << "$";
    } else {
        cout << restOfInput;
        outputFile << restOfInput;
    }
    cout << endl;
    outputFile << endl;
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
    
    cout << "Beginning parse of input string" << endl;
    outputFile << "Beginning parse of input string" << endl;
    
    displayStack(parseStack, input, position);
    cout << "Current token: " << currentToken << endl;
    outputFile << "Current token: " << currentToken << endl;
    
    // Continue until we've processed all input or encountered an error
    while (!parseStack.empty()) {
        string topStack = parseStack.top();
        parseStack.pop();
        
        cout << "Action: Pop " << topStack << " from stack" << endl;
        outputFile << "Action: Pop " << topStack << " from stack" << endl;
        
        // If we've reached the end of the stack and the input
        if (topStack == "$" && currentToken == "$") {
            cout << "Parse successful!" << endl;
            outputFile << "Parse successful!" << endl;
            return true;
        }
        
        // If the top of the stack matches the current input token
        if (isTerminal(topStack)) {
            if (topStack == currentToken) {
                cout << "Match: " << topStack << endl;
                outputFile << "Match: " << topStack << endl;
                
                // Advance to next token in input
                currentToken = getNextToken(input, position);
                cout << "Current token: " << currentToken << endl;
                outputFile << "Current token: " << currentToken << endl;
            } else if (topStack == "epsilon") {
                // Special case for epsilon, don't consume input
                cout << "Applying epsilon-rule (no input consumed)" << endl;
                outputFile << "Applying epsilon-rule (no input consumed)" << endl;
            } else {
                // Error: Terminal mismatch
                cout << "Syntax Error: Expected '" << topStack << "', but found '" << currentToken << "'" << endl;
                outputFile << "Syntax Error: Expected '" << topStack << "', but found '" << currentToken << "'" << endl;
                
                cout << "Line " << lineNumber << ": Syntax Error: Unexpected token '" << currentToken << "'" << endl;
                outputFile << "Line " << lineNumber << ": Syntax Error: Unexpected token '" << currentToken << "'" << endl;
                
                // Error recovery: Skip this token and continue
                currentToken = getNextToken(input, position);
                error = true;
                
                // Push the expected terminal back onto the stack for another try
                parseStack.push(topStack);
                
                cout << "Error recovery: Skipping token and continuing" << endl;
                outputFile << "Error recovery: Skipping token and continuing" << endl;
            }
        } else if (isNonTerminal(topStack)) {
            // Look up the production in the parse table
            if (parseTable.find({topStack, currentToken}) != parseTable.end()) {
                vector<string> production = parseTable[{topStack, currentToken}];
                
                cout << "Apply production: " << topStack << " -> ";
                outputFile << "Apply production: " << topStack << " -> ";
                
                for (const auto& symbol : production) {
                    cout << symbol << " ";
                    outputFile << symbol << " ";
                }
                cout << endl;
                outputFile << endl;
                
                // Push production in reverse order onto the stack
                if (!(production.size() == 1 && production[0] == "epsilon")) {
                    for (int i = production.size() - 1; i >= 0; i--) {
                        parseStack.push(production[i]);
                    }
                }
            } else {
                // Error: No rule in parse table
                cout << "Syntax Error: No production for [" << topStack << ", " << currentToken << "]" << endl;
                outputFile << "Syntax Error: No production for [" << topStack << ", " << currentToken << "]" << endl;
                
                cout << "Line " << lineNumber << ": Syntax Error: Unexpected token '" << currentToken 
                     << "', no production rule found" << endl;
                outputFile << "Line " << lineNumber << ": Syntax Error: Unexpected token '" << currentToken 
                           << "', no production rule found" << endl;
                
                // Error recovery: Try to skip this problematic input
                currentToken = getNextToken(input, position);
                error = true;
                
                // Try another production by checking FOLLOW set
                bool recovered = false;
                for (const auto& followTerm : followSets[topStack]) {
                    if (parseTable.find({topStack, followTerm}) != parseTable.end()) {
                        cout << "Error recovery: Trying production for FOLLOW symbol: " << followTerm << endl;
                        outputFile << "Error recovery: Trying production for FOLLOW symbol: " << followTerm << endl;
                        
                        vector<string> recoveryProduction = parseTable[{topStack, followTerm}];
                        
                        // Push recovery production onto stack
                        if (!(recoveryProduction.size() == 1 && recoveryProduction[0] == "epsilon")) {
                            for (int i = recoveryProduction.size() - 1; i >= 0; i--) {
                                parseStack.push(recoveryProduction[i]);
                            }
                        }
                        recovered = true;
                        break;
                    }
                }
                
                if (!recovered) {
                    // If no recovery possible, just skip this non-terminal
                    cout << "Error recovery: Skipping non-terminal " << topStack << endl;
                    outputFile << "Error recovery: Skipping non-terminal " << topStack << endl;
                }
            }
        } else if (topStack == "$" && currentToken != "$") {
            // End of stack but not end of input
            cout << "Syntax Error: Reached end of stack but input remains: " << currentToken << endl;
            outputFile << "Syntax Error: Reached end of stack but input remains: " << currentToken << endl;
            
            cout << "Line " << lineNumber << ": Syntax Error: Unexpected extra input" << endl;
            outputFile << "Line " << lineNumber << ": Syntax Error: Unexpected extra input" << endl;
            
            error = true;
            break;
        }
        
        // Display current stack and input
        displayStack(parseStack, input, position);
    }
    
    if (currentToken != "$") {
        cout << "Parse error: End of stack reached but input remains" << endl;
        outputFile << "Parse error: End of stack reached but input remains" << endl;
        return false;
    }
    
    return !error;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " grammar.txt input.txt output.txt" << endl;
        return 1;
    }
    
    CFGProcessor processor(argv[1], argv[3]);
    processor.displayResults();
    
    // Parse the input strings
    processor.parseInputFile(argv[2]);
    
    cout << "All done! Check out the results in " << argv[3] << endl;
    
    return 0;
}