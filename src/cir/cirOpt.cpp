/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
   // const 0 should not be considered
   for (size_t i = 1, n = _gateList.size(); i < n; ++i) {
      CirGate*& g = _gateList[i];
      if (!g) continue;
      if (g->inDfs()) continue;

      if (g->getType() == AIG_GATE || g->getType() == UNDEF_GATE) {
         cout << "Sweeping: " << g->getTypeStr() << "(" 
               << g->getID() << ") removed..." << endl;
         change(g);
      }
   }
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
   bool inv;
   int cnt, other;
   unsigned id;
   CirGate* replace = 0;
   for (size_t i = 0, n = _dfsList.size(); i < n; ++i) {
      if (_dfsList[i]->getType() != AIG_GATE) continue;

      id = _dfsList[i]->getID();
      cnt = 0;
      inv = false;
      replace = 0;
      while (!replace && cnt < 2) {
         if (_dfsList[i]->_faninPtr[cnt]->getType() == CONST_GATE) {
            // const 1
            other = cnt == 0? 1: 0;
            if (_dfsList[i]->invert(cnt)) {
               // replace with another fanin
               replace = _dfsList[i]->_faninPtr[other];
               inv = _dfsList[i]->invert(other);
            }
            // const 0
            else {
               replace = _gateList[0];
            }
            break;
         }
         ++cnt;
      }
      if (!replace && _dfsList[i]->_faninPtr[0] == _dfsList[i]->_faninPtr[1]) {
         // identical
         if (_dfsList[i]->invert(0) == _dfsList[i]->invert(1)) {
            // replace with any one of fanins
            replace = _dfsList[i]->_faninPtr[0];
            inv = _dfsList[i]->invert(0);
         }
         // invert
         else {
            // replace with const 0
            replace = _gateList[0];
         }
      }
      if (replace) {
         cout << "Simplifying: " << replace->getID() << " merging ";
         if (inv) cout << "!";
         cout << id << "..." << endl;
         change(_gateList[id], replace, inv);
      }
   }
   buildDfs(); // update dfs list
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
void
CirMgr::change(CirGate*& del, CirGate* replace, bool inv)
{
   // clearing del's fanins' fanouts
   for (size_t i = 0, n = del->_faninPtr.size(); i < n; ++i) {
      CirGate* t = del->_faninPtr[i];
      int index = del->findFanoutIndex(t);
      t->_fanoutPtr.erase(t->_fanoutPtr.begin() + index);
      t->_fanout.erase(t->_fanout.begin() + index);
   }
   // adding replace's fanouts
   if (replace) {
      for (size_t i = 0, n = del->_fanoutPtr.size(); i < n; ++i) {
         replace->_fanout.push_back(del->_fanoutPtr[i]->getID());
         replace->_fanoutPtr.push_back(del->_fanoutPtr[i]);
      }
   }
   // maintain del's fanouts' fanins
   for (size_t i = 0, n = del->_fanoutPtr.size(); i < n; ++i) {
      CirGate* t = del->_fanoutPtr[i];
      int index = del->findFaninIndex(t);
      if (replace) {
         t->_fanin[index] = replace->getID();
         t->_faninPtr[index] = replace;
         if (inv) t->inverseInvert(index);
      }
      else {
         t->_faninPtr.erase(t->_faninPtr.begin() + index);
         t->_fanin.erase(t->_fanin.begin() + index);
         t->deleteInvert(index);
      }
   }

   if (del->getType() == AIG_GATE) --_header.a;
   delete del;
   del = 0;
}