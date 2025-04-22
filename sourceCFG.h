#ifndef SOURCE_CFG_H
#define SOURCE_CFG_H

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

struct Grammar {
    std::set<std::string> terminals;     
    std::set<std::string> nonTerminals;  
    std::map<std::string, std::vector<std::vector<std::string>>> productions; // Maps each non-terminal to its production rules
    std::string startSymbol;        
};

class CFGProcessor {
private:
    Grammar grammar;
    std::map<std::string, std::set<std::string>> firstSets;    
    std::map<std::string, std::set<std::string>> followSets;   
    std::map<std::pair<std::string, std::string>, std::vector<std::string>> parseTable;
    bool isTerminal(const std::string& symbol);
    bool isNonTerminal(const std::string& symbol);
    std::set<std::string> computeFirstOfString(const std::vector<std::string>& symbols);
    
public:
    std::ofstream outputFile;
    CFGProcessor(const std::string& filename, const std::string& outputFilename);
    ~CFGProcessor();
    void displayGrammar(const Grammar& g);
    void performLeftFactoring();
    void eliminateLeftRecursion();
    void computeFirstSets();
    void computeFollowSets();
    void constructParseTable();
    void displayResults();
    
    // Parse input methods - note these aren't implemented in the shown code
    void parseInputFile(const std::string& inputFilename);
    bool parseString(const std::string& input, int lineNumber);
    std::string getNextToken(const std::string& input, int& position);
    void displayStack(std::stack<std::string> s, const std::string& input, int position);
};

#endif // SOURCE_CFG_H