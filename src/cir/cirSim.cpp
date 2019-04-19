/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <cmath>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
   RandomNumGen ran(9487);
   size_t num = 0;
   unsigned fail = 0;
   unsigned maxF = maxFail(_dfsList.size());
   size_t pre = 0, cur = 0;
   while (fail < maxF) {
      size_t r = 0;
      for (size_t i = 0, n = _PIOrder.size(); i < n; ++i) {
         r = ran(INT_MAX);
         // 64 bits device
         if (SIZE_T == 64) {
            r <<= 32;
            r += ran(INT_MAX);
         }
         _gateList[_PIOrder[i]]->setSimValue(r);
      }
      cur = simulate(true);
      if (cur == pre) ++fail;
      else fail = 0;
      pre = cur;
      writeLog();
      num += 64;
      if (cur == 0) break;
   }
   sort(_fecGrps.begin(), _fecGrps.end(), [](GateList* a, GateList* b) { return a->at(0)->getID() < b->at(0)->getID(); });
   setFecGrp();
   cout << char(13) << "                                        " << char(13) << num 
         << " patterns simulated." << endl;
}

void
CirMgr::fileSim(ifstream& patternFile)
{
   int cnt = 0;
   vector<size_t> pattern(_header.i, 0);
   string input;
   while (patternFile >> input) {
      if (input.size() != _header.i) {
         cerr << "\nError: Pattern(" << input << ") length(" << input.size() 
         << ") does not match the number of inputs(" << _header.i << ") in a circuit!!" << endl;
         sort(_fecGrps.begin(), _fecGrps.end(), [](GateList* a, GateList* b) { return a->at(0)->getID() < b->at(0)->getID(); });
         setFecGrp();
         cout << char(13) << SIZE_T * (cnt / SIZE_T) << " patterns simulated." << endl;
         return;
      }
      for (size_t i = 0, n = input.size(); i < n; ++i) {
         if (input[i] == '1') pattern[i] += ((size_t)1 << (cnt % SIZE_T));
         else if (input[i] != '0') { // illegal input
            cerr << "\nError: Pattern(" << input << ") contains a non-0/1 character(\'" << input[i] << "\')." << endl;
            sort(_fecGrps.begin(), _fecGrps.end(), [](GateList* a, GateList* b) { return a->at(0)->getID() < b->at(0)->getID(); });
            setFecGrp();
            cout << char(13) << SIZE_T * (cnt / SIZE_T) << " patterns simulated." << endl;
            return;
         }
      }
      if (!(++cnt % SIZE_T)) {
         for (size_t i = 0, n = _PIOrder.size(); i < n; ++i) {
            _gateList[_PIOrder[i]]->setSimValue(pattern[i]);
            pattern[i] = 0;
         }
         simulate(true);
         writeLog();
      }
   }
   if (cnt % SIZE_T) {
      for (size_t i = 0, n = _PIOrder.size(); i < n; ++i) {
         _gateList[_PIOrder[i]]->setSimValue(pattern[i]);
      }
      simulate(true);
      writeLog(cnt % SIZE_T);
   }
   sort(_fecGrps.begin(), _fecGrps.end(), [](GateList* a, GateList* b) { return a->at(0)->getID() < b->at(0)->getID(); });
   setFecGrp();
   cout << char(13) << cnt << " patterns simulated." << endl;
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/

size_t
CirMgr::simulate(bool p)
{
   // all gate
   initFecGrps();
   for (size_t i = 0, n = _dfsList.size(); i < n; ++i)
      _dfsList[i]->simulate(ALL_GATE);
   // event driven
   /*
   else {
      resetFlag();
      for (size_t i = 0, n = _dfsList.size(); i < n; ++i)
         if (_dfsList[i]->getType() == PO_GATE)
            _dfsList[i]->simulate(EVENT_DRIVEN);
   }*/

   vector<GateList*> tmpList;
   for (size_t i = 0, n = _fecGrps.size(); i < n; ++i) {
      HashMap<HashKey, GateList*> newFecGrps(getHashSize(_fecGrps[i]->size()));
      for (size_t j = 0, m = _fecGrps[i]->size(); j < m; ++j) {
         HashKey key(_fecGrps[i]->at(j)->getSimValue());
         GateList *grp;
         if (newFecGrps.check(key, grp)) {
            grp->push_back(_fecGrps[i]->at(j));
         }
         else {
            grp = new GateList();
            grp->push_back(_fecGrps[i]->at(j));
            newFecGrps.forceInsert(key, grp);
            tmpList.push_back(grp);
         }
      }
   }
   for (size_t i = 0, n = _fecGrps.size(); i < n; ++i) {
      delete _fecGrps[i];
   }
   // remove single
   _fecGrps.clear();
   for (auto it = tmpList.begin(); it != tmpList.end(); ++it) {
      if ((*it)->size() < 2) {
         for (auto itt = (*it)->begin(); itt != (*it)->end(); ++itt) {
            (*itt)->setGrp(0, 0);
         }
         delete (*it);
      }
      else {
         _fecGrps.push_back(*it);
      }
   }
   tmpList.clear();
   
   size_t ss = _fecGrps.size();
   if (p) cout << char(13) << "Total #FEC Group = " << ss <<flush;

   return ss;
}

unsigned
CirMgr::maxFail(size_t s)
{
   if (s < 100) return 3;
   else return (unsigned)(log(s) / log(5));
}

bool
CirMgr::initFecGrps()
{
   if (!_fecGrps.empty()) return false;

   GateList* fecGrp = new GateList();
   fecGrp->push_back(_gateList[0]);
   for (size_t i = 0, n = _dfsList.size(); i < n; ++i) {
      if (_dfsList[i]->getType() == AIG_GATE) {
         fecGrp->push_back(_dfsList[i]);
      }
   }
   sort(fecGrp->begin(), fecGrp->end(), [](CirGate* a, CirGate* b) { return a->getID() < b->getID(); });
   _fecGrps.push_back(fecGrp);
   return true;
}

void
CirMgr::writeLog(size_t size)
{
   if (!_simLog) return;
   size_t mask = 1;
   for (size_t i = 0; i < size; ++i) {
      for (size_t j = 0, m = _PIOrder.size(); j < m; ++j) {
         if (_gateList[_PIOrder[j]]->getSimValue() & mask) *_simLog << 1;
         else *_simLog << 0;
      }
      *_simLog << ' ';
      for (size_t j = _header.m + 1, m = _gateList.size(); j < m; ++j) {
         if (_gateList[j]->getSimValue() & mask) *_simLog << 1;
         else *_simLog << 0;
      }
      *_simLog << endl;
      mask <<= 1;
   }
}

void
CirMgr::setFecGrp()
{
   for (size_t i = 0, n = _fecGrps.size(); i < n; ++i) {
      for (size_t j = 0, m = _fecGrps[i]->size(); j < m; ++j) {
         _fecGrps[i]->at(j)->setGrp(_fecGrps[i], j);
      }
   }
}
