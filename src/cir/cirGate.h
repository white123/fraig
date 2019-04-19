/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
public:
   CirGate() {}
   CirGate(unsigned id, unsigned line): _ID(id), _line(line) {}
   virtual ~CirGate() {}

   // Basic access methods
   virtual string getTypeStr() const = 0;
   virtual GateType getType() const = 0;
   unsigned getLineNo() const { return _line; }
   unsigned getID() const { return _ID; }
   virtual bool isAig() const = 0;

   // Printing functions
   virtual void printGate() const = 0;
   void reportGate() const;
   void reportFanin(int level);
   void reportFanout(int level);

   void setID(const unsigned& gid) { _ID = gid; }
   unsigned getLine() const { return _line; }
   void setLine(const unsigned& l) { _line = l; }

   bool isVisited() const { return _isVisited; }
   void setMark(bool flag) { _isVisited = flag; }

   void addFanout(const unsigned& gid) { _fanout.push_back(gid); }
   IdList::iterator findFanoutIndex(const unsigned& gid) {
      return find(_fanout.begin(), _fanout.end(), gid);
   }
   int findFanoutIndex(CirGate* b) {
      for (size_t i = 0, n = b->_fanoutPtr.size(); i < n; ++i) {
         if (this == b->_fanoutPtr[i]) {
            return i;
         }
      }
      return -1;
   }
   void addFanin(const unsigned& gid) { _fanin.push_back(gid); }
   IdList::iterator findFaninIndex(const unsigned& gid) {
      return find(_fanin.begin(), _fanin.end(), gid);
   }
   int findFaninIndex(CirGate* b) {
      for (size_t i = 0, n = b->_faninPtr.size(); i < n; ++i) {
         if (this == b->_faninPtr[i]) {
            return i;
         }
      }
      return -1;
   }

   IdList   _fanout;
   IdList   _fanin;
   GateList _faninPtr;
   GateList _fanoutPtr;

   void setSymbol(string s) {
      _s = s;
   }
   string getSymbol() const { return _s; }

   bool invert(const int& i = 0) const { return _invert[i]; }
   void inverseInvert(const int& i = 0) { _invert[i] = !_invert[i]; }
   void addInvert(const bool& i) { _invert.push_back(i); }
   vector<bool> getInvert() { return _invert; }
   void deleteInvert(const int& i) { _invert.erase(_invert.begin() + i); }

   bool inDfs() const { return _inDfs; }
   void setDfs(bool f) const { _inDfs = f; }

   void setSimValue(const size_t& v) { _simValue = v; }
   size_t getSimValue() { return _simValue; }
   virtual bool simulate(SimType) = 0;
   void setGrp(GateList* g, unsigned id) { _fecGrp = g, _grpID = id; }

   Var getVar() { return _var; }
   void setVar(const Var& v) { _var = v; }

   bool isFraiged() { return _isFraiged; }
   void setFraiged(const bool& f) { _isFraiged = f; }

   GateList *   _fecGrp = 0;
   GateList *fecGrp() { return _fecGrp; }
   void deleteSelf();

private:
   unsigned int _ID;
   unsigned int _line;
   unsigned int _grpID = 0;
   mutable bool _isVisited = false;
   mutable bool _isReported = false;
   mutable bool _inDfs = false;
   mutable bool _isFraiged = false;
   Var          _var;

   void faninRecursive(int level, int indent, bool inv);
   void fanoutRecursive(int level, int indent, bool inv);

   string _s;

protected:
   vector<bool> _invert;
   size_t _simValue = 0;
   mutable bool _lastSim = false;
};

class PIGate : public CirGate
{
public:
   PIGate(unsigned id, unsigned line): CirGate(id, line) {}
   ~PIGate() {}

   GateType getType() const { return PI_GATE; }

   string getTypeStr() const { return "PI"; }
   void printGate() const {
      cout << getTypeStr() << "  " << getID();
      // symbol
      if (getSymbol().length() > 0) {
         cout << " (" << getSymbol() << ")";
      }
      cout << endl;
   }

   bool isAig() const { return false; }
   bool simulate(SimType) { return false; }
   
private:
   
};

class POGate : public CirGate
{
public:
   POGate(unsigned id, unsigned line): CirGate(id, line) {}
   ~POGate() {}
   
   GateType getType() const { return PO_GATE; }

   string getTypeStr() const { return "PO"; }
   void printGate() const {
      cout << getTypeStr() << "  " << getID() << " ";

      if (_faninPtr[0]->getType() == UNDEF_GATE) cout << "*";
      if (_invert[0]) cout << "!";
      cout << _fanin[0];
      // symbol
      if (getSymbol().length() > 0) {
         cout << " (" << getSymbol() << ")";
      }
      cout << endl;
   }

   bool isAig() const { return false; }
   bool simulate(SimType);   

private:

};

class AIGGate : public CirGate
{
public:
   AIGGate(unsigned id, unsigned line): CirGate(id, line) {}
   ~AIGGate() {}

   GateType getType() const { return AIG_GATE; }
   
   string getTypeStr() const { return "AIG"; }
   void printGate() const {
      cout << getTypeStr() << " " << getID();
      for (int i = 0; i < 2; ++i) {
         cout << " ";
         if (_faninPtr[i]->getType() == UNDEF_GATE) cout << "*";
         if (_invert[i]) cout << "!";
         cout << _fanin[i];
      }
      cout << endl;
   }

   bool isAig() const { return true; }
   bool simulate(SimType);

private:

};

class ConstGate : public CirGate
{
public:
   ConstGate(): CirGate(0, 0) {}
   ~ConstGate() {}

   GateType getType() const { return CONST_GATE; }
   
   string getTypeStr() const { return "CONST"; }
   void printGate() const {
      cout << "CONST0" << endl;
   }

   bool isAig() const { return false; }
   bool simulate(SimType) { return false; }

private:
   
};

class UndefGate : public CirGate
{
public:
   UndefGate(const unsigned& id): CirGate(id, 0) {}
   ~UndefGate() {}

   GateType getType() const { return UNDEF_GATE; }
   
   string getTypeStr() const { return "UNDEF"; }
   void printGate() const {}

   bool isAig() const { return false; }
   bool simulate(SimType) { return false; }

private:
   
};

#endif // CIR_GATE_H
