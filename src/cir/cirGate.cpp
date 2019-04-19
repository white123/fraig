/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void
CirGate::reportGate() const
{
   /*
   ==================================================
    PO(25)”23GAT$PO”, line 9 =
   ==================================================
   */ 
   string output = getTypeStr();
   output += "(";
   output += to_string(getID());
   output += ")";
   // symbol
   if (getSymbol().length() > 0) {
      output += "\"";
      output += getSymbol();
      output += "\"";
   }
   output += ", line ";
   output += to_string(getLineNo());

   cout << "========================================" 
         << "========================================" << endl;
   cout << "= ";
   cout << output << endl;
   cout << "= FECs:";
   if (_fecGrp) {
      for (size_t i = 0, n = _fecGrp->size(); i < n; ++i) {
      if (_fecGrp->at(i) == this) continue;
      cout << " ";
      if (_fecGrp->at(i)->getSimValue() != _simValue) cout << "!";
      cout << _fecGrp->at(i)->getID();
      }
   }
   cout << endl;
   cout << "= Value: ";
   size_t mask = (size_t)1 << 63;
   for (int i = 0; i < 64; ++i) {
      if (i != 0 && !(i % 8)) cout << "_";
      if (_simValue & mask) cout << 1;
      else cout << 0;
      mask >>= 1;
   }
   cout << "\n========================================" 
         << "========================================" << endl;
}

void
CirGate::reportFanin(int level)
{
   assert (level >= 0);
   cirMgr->resetFlag();
   faninRecursive(level, 0, false);

}
void
CirGate::faninRecursive(int level, int indent, bool inv)
{
   assert (level >= 0);
   // indent
   for (auto i = 0; i < indent; ++i) {
      cout << "  ";
   }
   if (inv) { cout << "!"; }
   cout << getTypeStr() << " " << getID();
   if (level > 0) {
      if (isVisited() && _faninPtr.size() > 0) {
         cout << " (*)" << endl;
         return;
      }
      cout << endl;
      setMark(true);
      for (size_t i = 0, n = _fanin.size(); i < n; ++i) {
         bool v = (_invert[i]);
         _faninPtr[i]->faninRecursive(level - 1, indent + 1, v);
      }
   }
   else { cout << endl; }
}

void
CirGate::reportFanout(int level)
{
   assert (level >= 0);
   cirMgr->resetFlag();
   fanoutRecursive(level, 0, false);
}

void
CirGate::fanoutRecursive(int level, int indent, bool inv)
{
   assert (level >= 0);
   // indent
   for (auto i = 0; i < indent; ++i) {
      cout << "  ";
   }
   if (inv) { cout << "!"; }
   cout << getTypeStr() << " " << getID();
   if (level > 0) {
      if (isVisited() && _fanoutPtr.size() > 0) {
         cout << " (*)" << endl;
         return;
      }
      cout << endl;
      setMark(true);
      for (size_t i = 0, n = _fanout.size(); i < n; ++i) {
         bool v;
         for (size_t j = 0, m = _fanoutPtr[i]->_faninPtr.size(); j < m; ++j) {
            if (_fanoutPtr[i]->_faninPtr[j] == this) {
               v = _fanoutPtr[i]->_invert[j];
               break;
            }
         }
         _fanoutPtr[i]->fanoutRecursive(level - 1, indent + 1, v);
      }
   }
   else { cout << endl; }
}

bool
AIGGate::simulate(SimType type)
{
   if (type == EVENT_DRIVEN) {
      if (isVisited()) return _lastSim;
      setMark(true);
      _lastSim = _faninPtr[0]->simulate(EVENT_DRIVEN) + _faninPtr[1]->simulate(EVENT_DRIVEN);
      if (!_lastSim) return _lastSim;
   }
   size_t s0 = _faninPtr[0]->getSimValue();
   size_t s1 = _faninPtr[1]->getSimValue();
   if (invert(0)) s0 = ~s0;
   if (invert(1)) s1 = ~s1;
   
   if (_simValue == (s0 & s1)) {
      _lastSim = false;
   }
   else {
      _simValue = s0 & s1;
      _lastSim = true;
   }
   return _lastSim;
}

bool
POGate::simulate(SimType type)
{
   if (type == EVENT_DRIVEN) {
      _lastSim = _faninPtr[0]->simulate(EVENT_DRIVEN);
      if (!_lastSim) return _lastSim;
   }
   _simValue = _faninPtr[0]->getSimValue();
   if (invert(0)) _simValue = ~_simValue;
   return true;
}

void
CirGate::deleteSelf()
{
   if (!_fecGrp) return;
   _fecGrp->erase(find(_fecGrp->begin(), _fecGrp->end(), this));
   _fecGrp = 0;
   _grpID = 0;
}
