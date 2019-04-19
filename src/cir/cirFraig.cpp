/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
   HashMap<HashKey, CirGate*> hash(getHashSize(_dfsList.size()));
   unsigned id;
   for (size_t i = 0, n = _dfsList.size(); i < n; ++i) {
      if (_dfsList[i]->getType() != AIG_GATE) continue;

      id = _dfsList[i]->getID();
      HashKey k = getKey(_dfsList[i]);
      CirGate *mergeGate;
      if (hash.check(k, mergeGate)) { // merge
         cout << "Strashing: " << mergeGate->getID() 
               << " merging " << id << "..." << endl;
         change(_gateList[id], mergeGate);
      }
      else { // insert
         hash.forceInsert(k, _dfsList[i]);
      }
   }
   buildDfs();
}

void
CirMgr::fraig()
{
   SatSolver solver;
   vector<size_t> pattern(_header.i, 0);
   vector<GateList> mergeGate;
   unsigned cnt = 0;
   unsigned fail = 0;
   size_t pre = 0, cur = 0;
   while (_fecGrps.size() && fail < 5) {
      solver.reset();
      solver.initialize();
      setVar(solver);
      constProofModel(solver);
      resetFraiged();
      for (size_t i = 0, n = _dfsList.size(); i < n; ++i) {
         if (_dfsList[i]->isFraiged()) continue;
         if (_dfsList[i]->getType() != AIG_GATE) continue;
         if (!_dfsList[i]->fecGrp()) continue;

         _dfsList[i]->setFraiged(true);
         GateList *list = _dfsList[i]->_fecGrp;
         if (list->at(0)->getType() == CONST_GATE) {
            bool inv = _dfsList[i]->getSimValue() != list->at(0)->getSimValue();
            if (proveSat(_dfsList[i], list->at(0), solver, inv)) { // not equal
               for (size_t k = 0, l = _PIOrder.size(); k < l; ++k) {
                  pattern[k] <<= 1;
                  pattern[k] += solver.getValue(_gateList[_PIOrder[k]]->getVar());
               }
               if (!(++cnt % SIZE_T)) { // resimulate
                  for (size_t k = 0, l = _PIOrder.size(); k < l; ++k) {
                     _gateList[_PIOrder[k]]->setSimValue(pattern[k]);
                     pattern[k] = 0;
                  }
                  simulate();
                  sort(_fecGrps.begin(), _fecGrps.end(), [](GateList* a, GateList* b) { return a->at(0)->getID() < b->at(0)->getID(); });
                  setFecGrp();
                  cout << "Updating by SAT... Total #FEC Group = " << _fecGrps.size() << endl;
                  break;
               }
            }
            else { // equal
               GateList pair;
               pair.push_back(_dfsList[i]);
               pair.push_back(list->at(0));
               mergeGate.push_back(pair);
            }
         }
         else {
            for (size_t j = 0, m = list->size(); j < m; ++j) {
               if (list->at(j) == _dfsList[i]) continue;
               if (list->at(j)->isFraiged()) continue;
               
               bool inv = _dfsList[i]->getSimValue() != list->at(j)->getSimValue();
               if (proveSat(_dfsList[i], list->at(j), solver, inv)) { // not equal
                  for (size_t k = 0, l = _PIOrder.size(); k < l; ++k) {
                     pattern[k] <<= 1;
                     pattern[k] += solver.getValue(_gateList[_PIOrder[k]]->getVar());
                  }
                  if (!(++cnt % SIZE_T)) { // resimulate
                     for (size_t k = 0, l = _PIOrder.size(); k < l; ++k) {
                        _gateList[_PIOrder[k]]->setSimValue(pattern[k]);
                        pattern[k] = 0;
                     }
                     simulate();
                     sort(_fecGrps.begin(), _fecGrps.end(), [](GateList* a, GateList* b) { return a->at(0)->getID() < b->at(0)->getID(); });
                     setFecGrp();
                     cout << "Updating by SAT... Total #FEC Group = " << _fecGrps.size() << endl;
                     break;
                  }
               }
               else { // equal
                  GateList pair;
                  pair.push_back(list->at(j));
                  pair.push_back(_dfsList[i]);
                  mergeGate.push_back(pair);
               }
            }
         }
      }
      // merge gate
      for (size_t i = 0, n = mergeGate.size(); i < n; ++i) {
         bool inv = mergeGate[i][0]->getSimValue() != mergeGate[i][1]->getSimValue();
         cout << "Fraig: " << mergeGate[i][1]->getID() << " merging "
            << (inv? "!": "") << mergeGate[i][0]->getID() << "..." << endl;
         mergeGate[i][0]->deleteSelf();
         change(_gateList[mergeGate[i][0]->getID()], mergeGate[i][1], inv);
      }
      mergeGate.clear();
      buildDfs();
      // update fec group
      vector<GateList*> tmp;
      for (int i = (int)_fecGrps.size() - 1; i >= 0; --i) {
         if (_fecGrps[i]->size() < 2) {
            if (!_fecGrps[i]->empty()) _fecGrps[i]->at(0)->setGrp(0, 0);
            delete _fecGrps[i];
         }
         else {
            tmp.push_back(_fecGrps[i]);
         }
      }
      _fecGrps.swap(tmp);
      cout << "Updating by UNSAT... Total #FEC Group = " << _fecGrps.size() << endl;
      
      // resimulate after each run
      if (cnt % SIZE_T) {
         for (size_t k = 0, l = _PIOrder.size(); k < l; ++k) {
            _gateList[_PIOrder[k]]->setSimValue(pattern[k]);
            pattern[k] = 0;
         }
         cnt = 0;
         simulate();
         sort(_fecGrps.begin(), _fecGrps.end(), [](GateList* a, GateList* b) { return a->at(0)->getID() < b->at(0)->getID(); });
         setFecGrp();
         cout << "Updating by SAT... Total #FEC Group = " << _fecGrps.size() << endl;
      }
      cur = _fecGrps.size();
      if (pre == cur) ++fail;
      else fail = 0;
      pre = cur;
   }
   resetFecGrps();
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
HashKey
CirMgr::getKey(CirGate* g) const {
   HashKey k(g->invert(0), g->invert(1), g->_fanin[0], g->_fanin[1]);
   return k;
}

void
CirMgr::setVar(SatSolver& s)
{
   for (size_t i = 0, n = _dfsList.size(); i < n; ++i) {
      if (_dfsList[i]->getType() == PO_GATE) _dfsList[i]->setVar(_dfsList[i]->_faninPtr[0]->getVar());
      else _dfsList[i]->setVar(s.newVar());
   }
}
void
CirMgr::constProofModel(SatSolver& s)
{
   for (size_t i = 0, n = _dfsList.size(); i < n; ++i) {
      if (_dfsList[i]->getType() != AIG_GATE) continue;

      s.addAigCNF(_dfsList[i]->getVar(), 
                  _dfsList[i]->_faninPtr[0]->getVar(), _dfsList[i]->invert(0),
                  _dfsList[i]->_faninPtr[1]->getVar(), _dfsList[i]->invert(1));
   }
}

bool
CirMgr::proveSat(CirGate * a, CirGate * b, SatSolver& s, const bool& inv)
{
   bool isSat;
   if (b->getType() == CONST_GATE) { // if b is const 0, just prove a
      s.assumeRelease();
      s.assumeProperty(a->getVar(), !inv);
      isSat = s.assumpSolve();
      cout << "                                        " << char(13) << flush
         << "Proving " << a->getID() << " = " << (inv? "!": "") << (inv? "1": "0")
         << "..." << (isSat? "SAT": "UNSAT") << "!!" << char(13) << flush;
   }
   else {
      Var var = s.newVar();
      s.addXorCNF(var, a->getVar(), 0, b->getVar(), inv);
      s.assumeRelease();
      s.assumeProperty(var, true);
      isSat = s.assumpSolve();
      cout << "                                        " << char(13) << flush
         << "Proving (" << a->getID() << ", " << (inv? "!": "") << b->getID()
         << ")..." << (isSat? "SAT": "UNSAT") << "!!" << char(13) << flush;
   }
   if (!isSat) b->setFraiged(true);
   return isSat;
}

void
CirMgr::resetFecGrps()
{
   for (size_t i = 0, n = _fecGrps.size(); i < n; ++i) {
      if (_fecGrps[i]) delete _fecGrps[i];
   }
   _fecGrps.clear();
   for (size_t i = 0, n = _dfsList.size(); i < n; ++i) {
      _dfsList[i]->setGrp(0, 0);
   }
}