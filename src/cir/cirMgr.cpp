/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include <vector>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
/*static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}*/

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
   ifstream inf(fileName);
   string x;
   unsigned int line = 1;
   getline(inf, x);
   readHeader(x);
   ++line;
   for (unsigned int i = 0; i < _header.i; ++i) {
      getline(inf, x);
      readPI(x, line);
      ++line;
   }
   int PONo = 1;
   for (unsigned int i = 0; i < _header.o; ++i) {
      getline(inf, x);
      readPO(x, line, PONo);
      ++line; 
      ++PONo;
   }
   for (unsigned int i = 0; i < _header.a; ++i) {
      getline(inf, x);
      readAIG(x, line);
      ++line; 
   }

   // connection
   buildConnection();
   
   // symbol
   while (getline(inf, x)) {
      if (x == "c") { break; }
      readSymbol(x);
   }
   // comment
   while (getline(inf, x)) {
      readComment(x);
   }

   inf.close();
   return true;
}

void
CirMgr::readHeader(const string& x) {
   vector<string> tokens;
   string token;
   size_t n = myStrGetTok(x, token);
   while (token.size()) {
      tokens.push_back(token);
      n = myStrGetTok(x, token, n);
   }
   assert(tokens.size() == 6);
   _header.aag = tokens[0];
   _header.m = stoi(tokens[1]);
   _header.i = stoi(tokens[2]);
   _header.l = stoi(tokens[3]);
   _header.o = stoi(tokens[4]);
   _header.a = stoi(tokens[5]);

   int arraySize = _header.m + _header.o + 1;
   for (auto i = 0; i < arraySize; ++i) {
      _gateList.push_back(NULL);
   }
   _gateList[0] = new ConstGate();
}
void
CirMgr::readPI(const string& x, const unsigned& line) {
   string token;
   size_t n = myStrGetTok(x, token);
   if (n != string::npos) {
      // handle error
      return;
   }
   int id = stoi(token) / 2;
   _gateList[id] = new PIGate(id, line);
   _PIOrder.push_back(id);
}
void
CirMgr::readPO(const string& x, const unsigned& line, const unsigned& No) {
   string token;
   size_t n = myStrGetTok(x, token);
   if (n != string::npos) {
      // handle error
      return;
   }
   int id = _header.m + No;
   _gateList[id] = new POGate(id, line);

   int faninLId = stoi(token);
   _gateList[id]->addFanin(faninLId / 2);
   _gateList[id]->addInvert(faninLId % 2 == 1);
}
void
CirMgr::readAIG(const string& x, const unsigned& line) {
   vector<string> tokens;
   string token;
   size_t n = myStrGetTok(x, token);
   while (token.size()) {
      tokens.push_back(token);
      n = myStrGetTok(x, token, n);
   }
   assert(tokens.size() == 3);
   
   int id = stoi(tokens[0]) / 2;
   _gateList[id] = new AIGGate(id, line);

   for (int i = 1; i < 3; ++i) {
      int faninLId = stoi(tokens[i]);
      _gateList[id]->addFanin(faninLId / 2);
      _gateList[id]->addInvert(faninLId % 2 == 1);
   }
}
void
CirMgr::readSymbol(const string& x) {
   string token;
   myStrGetTok(x, token);

   string mode = token.substr(0, 1);
   size_t pos = x.find(' ');
   string s = x.substr(pos + 1, x.length() - pos);

   int num = stoi(token.substr(1, token.length() - 1));
   if (mode == "i") {
      int id = _PIOrder[num];
      _gateList[id]->setSymbol(s);
   }
   else if (mode == "o") {
      _gateList[_header.m + num + 1]->setSymbol(s);
   }
}
void
CirMgr::readComment(const string& x) {
   if (!_comment.empty()) _comment += '\n';
   _comment += x;
}
void
CirMgr::buildConnection() {
   for (size_t i = 0, n = _gateList.size(); i < n; ++i) {
      if (_gateList[i]) {
         for (auto id = _gateList[i]->_fanin.begin(); id != _gateList[i]->_fanin.end(); ++id) {
            if (!_gateList[*id]) { // undefine
               _gateList[*id] = new UndefGate(*id);
            }
            _gateList[*id]->addFanout(_gateList[i]->getID());
            _gateList[*id]->_fanoutPtr.push_back(_gateList[i]);
            _gateList[i]->_faninPtr.push_back(_gateList[*id]);
         }
      }
   }
   buildDfs();
}
void
CirMgr::buildDfs() {
   resetFlag();
   resetDFS();
   _dfsList.clear();
   for (size_t i = _header.m + 1, n = _gateList.size(); i < n; ++i) {
      buildDfs(_gateList[i]);
   }
}
void
CirMgr::buildDfs(CirGate* g) {
   if (g->isVisited()) return;
   if (g->getType() == UNDEF_GATE) { // set true but don't push back to dfs list
      g->setDfs(true);
      return;
   }

   for (size_t i = 0, n = g->_faninPtr.size(); i < n; ++i) {
      buildDfs(g->_faninPtr[i]);
   }
   
   _dfsList.push_back(g);

   g->setDfs(true);
   g->setMark(true);
}


/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
   cout << endl;
   cout << "Circuit Statistics\n" 
         << "==================\n";
   cout << "  " << setiosflags(ios::left) << setw(5) << "PI" 
            << resetiosflags(ios::left) << setw(9) << _header.i << endl;
   cout << "  " << setiosflags(ios::left) << setw(5) << "PO" 
            << resetiosflags(ios::left) << setw(9) << _header.o << endl;
   cout << "  " << setiosflags(ios::left) << setw(5) << "AIG" 
            << resetiosflags(ios::left) << setw(9) << _header.a << endl;
   cout << "------------------" << endl;
   cout << "  " << setiosflags(ios::left) << setw(5) << "Total" 
            << resetiosflags(ios::left) << setw(9) << _header.i+_header.o+_header.a << endl;
}

void
CirMgr::printNetlist() const
{
   cout << endl;
   for (unsigned i = 0, n = _dfsList.size(); i < n; ++i) {
      cout << "[" << i << "] ";
      _dfsList[i]->printGate();
   }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for (auto it = _PIOrder.begin(); it != _PIOrder.end(); ++it) {
      cout << " " << *it;
   }
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for (size_t i = _header.m + 1, n = _gateList.size(); i < n; ++i) {
      cout << " " << i;
   }
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
   vector<unsigned> fl;
   vector<unsigned> notUsed;
   for (size_t i = 1, n = _gateList.size(); i < n; ++i) {
      if (_gateList[i]) {
         // float gate
         if (_gateList[i]->_faninPtr.size() != 2 && _gateList[i]->getType() == AIG_GATE)
            fl.push_back(_gateList[i]->getID());
         
         for (size_t j = 0, m = _gateList[i]->_faninPtr.size(); j < m; ++j) {
            if (_gateList[i]->_faninPtr[j]->getType() == UNDEF_GATE) {
               fl.push_back(_gateList[i]->getID());
               break;
            }
         }

         // not used gate
         if (_gateList[i]->_fanoutPtr.size() == 0) {
            if (_gateList[i]->getType() != CONST_GATE && _gateList[i]->getType() != PO_GATE) {
               notUsed.push_back(_gateList[i]->getID());
            }
         }
      }
   }
   if (fl.size() > 0) {
      cout << "Gates with floating fanin(s):";
      for (auto it = fl.begin(); it != fl.end(); ++it) {
         cout << " " << *it;
      }
      cout << endl;
   }
   if (notUsed.size() > 0) {
      cout << "Gates defined but not used  :";
      for (auto it = notUsed.begin(); it != notUsed.end(); ++it) {
         cout << " " << *it;
      }
      cout << endl;
   }
}

void
CirMgr::printFECPairs() const
{
   for (size_t i = 0, n = _fecGrps.size(); i < n; ++i) {
      cout << '[' << i << ']';
      for (size_t j = 0, m = _fecGrps[i]->size(); j < m; ++j) {
         cout << ' ';
         if (_fecGrps[i]->at(j)->getSimValue() != _fecGrps[i]->at(0)->getSimValue())
            cout << '!';
         cout << _fecGrps[i]->at(j)->getID();
      }
      cout << endl;
   }
}

void
CirMgr::writeAag(ostream& outfile) const
{
   // header
   outfile << "aag " << _header.m << " " << _header.i 
            << " " << _header.l << " " << _header.o << " ";
   vector<CirGate*> aig;
   resetFlag();
   for (size_t i = _header.m + 1, n = _gateList.size(); i < n; ++i) {
      dfsAIG(_gateList[i], aig);
   }
   outfile << aig.size() << endl;

   // PI
   for (size_t i = 0; i < _PIOrder.size(); ++i) {
      outfile << _PIOrder[i] * 2;
      outfile << endl;
   }

   // PO
   for (size_t i = _header.m + 1, n = _gateList.size(); i < n; ++i) {
      unsigned o = _gateList[i]->_fanin[0] * 2;
      if (_gateList[i]->invert(0)) { ++o; }
      outfile << o << endl;
   }

   // AIG
   for (size_t i = 0; i < aig.size(); ++i) {
      unsigned o[2];
      for (int j = 0; j < 2; ++j) {
         o[j] = aig[i]->_fanin[j] * 2;
         if (aig[i]->invert(j)) { ++o[j]; }
      }
      outfile << aig[i]->getID() * 2 << " "
            << o[0] << " " << o[1] << endl;
   }

   // Symbol
   for (size_t i = 0; i < _PIOrder.size(); ++i) {
      int id = _PIOrder[i];
      if (_gateList[id]->getSymbol().length() > 0) {
         outfile << "i" << i << " " << _gateList[id]->getSymbol() << endl;
      }
   }
   for (size_t i = _header.m + 1, n = _gateList.size(); i < n; ++i) {
      if (_gateList[i]->getSymbol().length() > 0) {
         outfile << "o" << i - (_header.m + 1) << " " << _gateList[i]->getSymbol() << endl;
      }  
   }

   // Comment
   outfile << "c" << endl;
   outfile << "AAG output by Weiting Tang" << endl;
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
   // header
   resetFlag();
   vector<CirGate*> d;
   dfs(g, d);
   IdList pi, aig;
   unsigned ii = 0, aa = 0;
   for (size_t i = 0, n = d.size(); i < n; ++i) {
      if (d[i]->getType() == AIG_GATE) {
         ++aa;
         aig.push_back(d[i]->getID());
      }
      if (d[i]->getType() == PI_GATE) {
         ++ii;
         pi.push_back(d[i]->getID());
      }
   }
   outfile << "aag " << g->getID() << " " << ii << " " 
         << _header.l << " " << 1 << " " << aa << endl;
   
   // PI
   for (size_t i = 0, n = _PIOrder.size(); i < n; ++i) {
      for (size_t j = 0, m = pi.size(); j < m; ++j) {
         if (_PIOrder[i] == pi[j]){
            outfile << _PIOrder[i] * 2 << endl;
            break;
         }
      }
   }

   // PO
   outfile << g->getID() * 2 << endl;

   // AIG
   for (size_t i = 0; i < aig.size(); ++i) {
      unsigned o[2];
      for (int j = 0; j < 2; ++j) {
         o[j] = _gateList[aig[i]]->_fanin[j] * 2;
         if (_gateList[aig[i]]->invert(j)) { ++o[j]; }
      }
      outfile << _gateList[aig[i]]->getID() * 2 << " "
            << o[0] << " " << o[1] << endl;
   }

   // Symbol
   for (size_t i = 0, n = _PIOrder.size(); i < n; ++i) {
      for (size_t j = 0, m = pi.size(); j < m; ++j) {
         if (_PIOrder[i] == pi[j]){
            if (_gateList[_PIOrder[i]]->getSymbol().length() > 0) {
               outfile << "i" << i << " " << _gateList[_PIOrder[i]]->getSymbol() << endl;
            }
         }
      }
   }
   outfile << "o0 Gate_" << g->getID() << endl;

   // Comment
   outfile << "c" << endl;
   outfile << "Write gate(" << g->getID() << ") by Weiting Tang" << endl;
}

void
CirMgr::dfsAIG(CirGate* g, GateList& a) const {
   if (g->isVisited() || g->getType() == UNDEF_GATE) return;
   for (size_t i = 0; i < g->_faninPtr.size(); ++i) {
      dfsAIG(g->_faninPtr[i], a);
   }
   
   if (g->getType() == AIG_GATE) {
      a.push_back(g);
   }

   g->setMark(true);
}

void
CirMgr::dfs(CirGate* g, GateList& a) const {
   if (g->isVisited() || g->getType() == UNDEF_GATE) return;
   for (size_t i = 0; i < g->_faninPtr.size(); ++i) {
      dfs(g->_faninPtr[i], a);
   }
   
   a.push_back(g);

   g->setMark(true);
}