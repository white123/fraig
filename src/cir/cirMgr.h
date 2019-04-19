/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"
#include "cirGate.h"

extern CirMgr *cirMgr;

class CirMgr
{
public:
   CirMgr() {}
   ~CirMgr() {
      for (size_t i = 0, n = _gateList.size(); i < n; ++i) {
         if (_gateList[i]) {
            delete _gateList[i];
         }
      }
   } 

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const {
      if (gid >= _gateList.size()) return 0;
      return _gateList[gid];
   }

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();
   void change(CirGate*& del, CirGate* replace = 0, bool inv = false);

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();
   HashKey getKey(CirGate* g) const;

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;
   void dfsAIG(CirGate*, vector<CirGate*>&) const;
   void dfs(CirGate*, vector<CirGate*>&) const;
   void resetFlag() const {
      for (size_t i = 0, n = _gateList.size(); i < n; ++i) {
         if (_gateList[i]) { _gateList[i]->setMark(false); }
      }
   }
   void resetDFS() const {
      for (size_t i = 0, n = _gateList.size(); i < n; ++i) {
         if (_gateList[i]) { _gateList[i]->setDfs(false); }
      }
   }
   void resetFraiged() const {
      for (size_t i = 0, n = _gateList.size(); i < n; ++i) {
         if (_gateList[i]) { _gateList[i]->setFraiged(false); }
      }
   }

   size_t getListSize() const { return _header.m + _header.o + 1; }

private:
   ofstream           *_simLog;
   GateList           _gateList;
   GateList           _dfsList;
   vector<GateList*>  _fecGrps;

   struct Header {
      string aag;
      unsigned int m;
      unsigned int i;
      unsigned int l;
      unsigned int o;
      unsigned int a;
   };

   Header _header;
   void readHeader(const string& x);
   void readPI(const string& x, const unsigned& line);
   void readPO(const string& x, const unsigned& line, const unsigned& No);
   void readAIG(const string& x, const unsigned& line);
   void readSymbol(const string& x);
   void readComment(const string& x);
   void buildConnection();
   void buildDfs();
   void buildDfs(CirGate* g);
   void dfsTravel(CirGate* g, int& index) const;
   
   size_t simulate(bool p = false);
   unsigned maxFail(size_t);
   bool initFecGrps();
   void writeLog(size_t size = SIZE_T);
   void setFecGrp();

   void setVar(SatSolver&);
   void constProofModel(SatSolver&);
   bool proveSat(CirGate *, CirGate *, SatSolver&, const bool&);
   void resetFecGrps();

   vector<unsigned> _PIOrder;

   string _comment;
};

#endif // CIR_MGR_H
