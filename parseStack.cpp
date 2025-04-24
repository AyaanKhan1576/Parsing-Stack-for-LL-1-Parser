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

/* -----------------------------------------------------------------
   ───────  Pretty-printing helpers (unchanged from last version) ───────
------------------------------------------------------------------*/
static constexpr int COL_W = 40;
static constexpr int ACT_W = 30;

void CFGProcessor::printTableHeader()
{
    const string sep = "+" + string(COL_W, '-') + "+" +
                       string(COL_W, '-') + "+" +
                       string(ACT_W, '-') + "+";

    auto hdr = [&](ostream& os)
    {
        os << sep << '\n'
           << "|" << setw(COL_W) << left << " STACK"
           << "|" << setw(COL_W) << left << " INPUT"
           << "|" << setw(ACT_W) << left << " ACTION"
           << "|\n" << sep << '\n';
    };
    hdr(cout);
    if (outputFile.is_open()) hdr(outputFile);
}

void CFGProcessor::displayStack(stack<string> s,
                                const string& input,
                                int position,
                                const string& action)
{
    vector<string> tmp;
    while (!s.empty()) { tmp.push_back(s.top()); s.pop(); }

    string stackCol;
    for (int i = static_cast<int>(tmp.size()) - 1; i >= 0; --i)
        stackCol += tmp[i] + " ";
    if (stackCol.empty()) stackCol = "ε";

    /* build INPUT column */
    int tPos = position;  string tok, inpCol;
    while ((tok = getNextToken(input, tPos)) != "$") {
        inpCol += tok + " ";
        if (tPos >= static_cast<int>(input.length())) break;
    }
    inpCol += "$";

    auto row = [&](ostream& os)
    {
        os << "|" << setw(COL_W) << left << stackCol.substr(0, COL_W - 1)
           << "|" << setw(COL_W) << left << inpCol.substr(0, COL_W - 1)
           << "|" << setw(ACT_W) << left << action.substr(0, ACT_W - 1)
           << "|\n";
    };
    row(cout);
    if (outputFile.is_open()) row(outputFile);
}

/* -----------------------------------------------------------------
   ───────  File-level driver (changed!)  ───────
------------------------------------------------------------------*/

void CFGProcessor::parseInputFile(const string& inputFilename)
{
    ifstream fin(inputFilename);
    if (!fin.is_open()) {
        cerr << "Error opening input file: " << inputFilename << '\n';
        if (outputFile.is_open())
            outputFile << "Error opening input file: " << inputFilename << '\n';
        return;
    }

    cout << "\n===== PARSING INPUT STRINGS =====\n\n";
    if (outputFile.is_open())
        outputFile << "\n===== PARSING INPUT STRINGS =====\n\n";

    string line;  int lineNo = 0, totalErrors = 0;

    while (getline(fin, line))
    {
        lineNo++;
        if (line.empty() || line[0] == '#') continue;

        /* ——— Heading & original source line ——— */
        cout << "──────── Line " << lineNo << " ────────\n"
             << "Code  : " << line << "\n";
        if (outputFile.is_open()) {
            outputFile << "──────── Line " << lineNo << " ────────\n"
                       << "Code  : " << line << "\n";
        }

        bool ok = parseString(line, lineNo);

        /* ——— per-line result ——— */
        if (ok) {
            cout << "Result: Line " << lineNo << " parsed successfully.\n\n";
            if (outputFile.is_open())
                outputFile << "Result: Line " << lineNo
                           << " parsed successfully.\n\n";
        } else {
            cout << "Result: Line " << lineNo << " contained syntax error(s).\n\n";
            if (outputFile.is_open())
                outputFile << "Result: Line " << lineNo
                           << " contained syntax error(s).\n\n";
            totalErrors++;
        }
    }

    cout << "Parsing completed with " << totalErrors << " error(s).\n";
    if (outputFile.is_open())
        outputFile << "Parsing completed with " << totalErrors << " error(s).\n";
}

/* -----------------------------------------------------------------
   ───────  parseString (unchanged from last version)  ───────
------------------------------------------------------------------*/
bool CFGProcessor::parseString(const string& input, int lineNumber)
{
    stack<string> st;  st.push("$");  st.push(grammar.startSymbol);
    int pos = 0;  string la = getNextToken(input, pos);

    bool hadErr = false;  int errStreak = 0;  const int MAX_ERR = 10;

    printTableHeader();
    displayStack(st, input, pos, "Initial state");

    while (!st.empty() && errStreak < MAX_ERR)
    {
        string top = st.top();  string act;

        if (top == la) {
            if (top == "$") { act = "ACCEPT"; st.pop(); displayStack(st,input,pos,act); break; }
            st.pop(); act = "Match '" + top + "'"; la = getNextToken(input,pos); errStreak = 0;
        }
        else if (isTerminal(top)) {
            hadErr = true; act = "Error: expected '" + top + "'";
            st.pop(); la = getNextToken(input,pos); errStreak++;
        }
        else if (isNonTerminal(top)) {
            if (parseTable.count({top, la})) {
                vector<string> prod = parseTable[{top, la}]; st.pop();
                if (!(prod.size()==1 && prod[0]=="epsilon"))
                    for (int i=static_cast<int>(prod.size())-1;i>=0;--i) st.push(prod[i]);
                act = top + " → "; for (auto&s:prod) act+=s+" "; errStreak=0;
            } else {
                hadErr = true; act = "Error: no rule for ("+top+", "+la+")";
                la = getNextToken(input,pos); errStreak++;
            }
        } else { cerr<<"Internal parser error.\n"; return false; }

        displayStack(st,input,pos,act);
    }

    if (errStreak>=MAX_ERR) {
        cout<<"Too many consecutive errors – giving up on line "<<lineNumber<<".\n";
        if (outputFile.is_open())
            outputFile<<"Too many consecutive errors – giving up on line "<<lineNumber<<".\n";
        return false;
    }
    return !hadErr;
}

/*  getNextToken & main() unchanged – see previous file  */


/* -----------------------------------------------------------------
   ──────────────  Tokeniser (unchanged)  ──────────────
------------------------------------------------------------------*/

string CFGProcessor::getNextToken(const string& input, int& position)
{
    while (position < static_cast<int>(input.length()) &&
           isspace(static_cast<unsigned char>(input[position])))
        ++position;

    if (position >= static_cast<int>(input.length())) return "$";

    char c = input[position];

    /* single-char tokens */
    string one(1, c);
    const string singles = "(){}`;=+-*/";
    if (singles.find(c) != string::npos) { ++position; return one; }

    /* two-char operators */
    if ((c == '=' || c == '!' || c == '<' || c == '>') &&
        position + 1 < static_cast<int>(input.length()) &&
        input[position + 1] == '=') {
        position += 2;
        return input.substr(position - 2, 2);
    }

    /* standalone < or > */
    if (c == '<' || c == '>') { ++position; return one; }

    /* identifiers / keywords */
    if (isalpha(c) || c == '_') {
        int start = position;
        while (position < static_cast<int>(input.length()) &&
               (isalnum(input[position]) || input[position] == '_'))
            ++position;
        string tok = input.substr(start, position - start);
        if (grammar.terminals.count(tok)) return tok;   // keyword
        if (tok == "int" || tok == "if")    return tok; // hard-coded kw
        return "id";
    }

    /* integer literals */
    if (isdigit(c)) {
        while (position < static_cast<int>(input.length()) &&
               isdigit(input[position])) ++position;
        return grammar.terminals.count("int_lit") ? "int_lit"
                                                  : input.substr(position, 1);
    }

    /* unknown char */
    ++position;
    cerr << "Warning: unrecognised char '" << c << "'\n";
    return one;
}

/* -----------------------------------------------------------------
   ──────────────  Main  ──────────────
------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
    if (argc != 4) {
        cerr << "Usage: " << argv[0]
             << " grammar.txt input.txt output.txt\n";
        return 1;
    }

    CFGProcessor proc(argv[1], argv[3]);
    proc.displayResults();           // grammar → FIRST/FOLLOW/table
    proc.parseInputFile(argv[2]);    // now parse the supplied strings

    cout << "\nProcessing complete.  Results written to " << argv[3] << '\n';
    if (proc.outputFile.is_open())
        proc.outputFile << "\nProcessing complete.\n";
    return 0;
}
