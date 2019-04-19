/****************************************************************************
  FileName     [ myHashSet.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashSet ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2014-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_SET_H
#define MY_HASH_SET_H

#include <vector>

using namespace std;

//---------------------
// Define HashSet class
//---------------------
// To use HashSet ADT,
// the class "Data" should at least overload the "()" and "==" operators.
//
// "operator ()" is to generate the hash key (size_t)
// that will be % by _numBuckets to get the bucket number.
// ==> See "bucketNum()"
//
// "operator ==" is to check whether there has already been
// an equivalent "Data" object in the HashSet.
// Note that HashSet does not allow equivalent nodes to be inserted
//
template <class Data>
class HashSet
{
public:
   HashSet(size_t b = 0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
   ~HashSet() { reset(); }

   // TODO: implement the HashSet<Data>::iterator
   // o An iterator should be able to go through all the valid Data
   //   in the Hash
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   class iterator
   {
      friend class HashSet<Data>;
      
   public:
      iterator(typename vector<Data>::iterator n = 0, vector<Data>* b = 0,
         vector<Data>* h = 0, vector<Data>* e = 0):
         _it(n), _b(b), _head(h), _end(e) {}
      iterator(const iterator& i) : 
         _it(i._it), _b(i._b), _head(i._head), _end(i._end) {}
      ~iterator() {}

      const Data& operator * () const { return *_it; }
      iterator& operator ++ () {
         ++_it;
         while (_it == _b->end() && _b != _end) {
           ++_b;
           _it = _b->begin();
         }
         return (*this); 
      }
      iterator operator ++ (int) {  
         iterator tmp = *(this);
         ++(*this);
         return tmp;
      }
      iterator& operator -- () {
         if (_it != _b->begin()) {
            --_it;
            return (*this);
         }

         while (_it == _b->begin() && _b != _head) {
           --_b;
           _it = _b->end() - 1;
         }
         return (*this); 
      }
      iterator operator -- (int) { 
         iterator tmp = *(this);
         --(*this);
         return tmp;
      }

      iterator& operator = (const iterator& i) { 
         this->_it = i._it;
         this->_b = i._b;
         this->_head = i._head;
         this->_end = i._end;
         return *(this);
      }
      bool operator != (const iterator& i) const { 
        return !(*this == i); 
      }
      bool operator == (const iterator& i) const { 
        return ((this->_it == i._it) && (this->_b == i._b) && 
                  (this->_head == i._head) && (this->_end == i._end));
      }
      
   private:
      typename vector<Data>::iterator     _it;
      vector<Data>*                       _b;
      vector<Data>*                       _head;
      vector<Data>*                       _end;
   };

   void init(size_t b) { _numBuckets = b; _buckets = new vector<Data>[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   void clear() {
      for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<Data>& operator [] (size_t i) { return _buckets[i]; }
   const vector<Data>& operator [](size_t i) const { return _buckets[i]; }

   // TODO: implement these functions
   //
   // Point to the first valid data
   iterator begin() const {
      for (size_t i = 0; i < _numBuckets; ++i) {
         if (_buckets[i].size())
            return iterator(_buckets[i].begin(), _buckets + i,
                              _buckets, _buckets + _numBuckets);
      }
      return end();
   }
   // Pass the end
   iterator end() const {
      return iterator(_buckets[_numBuckets].begin(), 
                        _buckets + _numBuckets,
                        _buckets, _buckets + _numBuckets);
   }
   // return true if no valid data
   bool empty() const { return begin() == end(); }
   // number of valid data
   size_t size() const {
      size_t s = 0;
      vector<Data>* v = _buckets;
      for (int i = 0; i < _numBuckets; ++i) {
         s += v[i].size();
      }
      return s;
   }

   // check if d is in the hash...
   // if yes, return true;
   // else return false;
   bool check(const Data& d) const {
      size_t n = bucketNum(d);
      auto it = _buckets[n].begin();
      for (; it != _buckets[n].end(); ++it) {
         if (*it == d) { return true; }
      }
      return false;
   }

   // query if d is in the hash...
   // if yes, replace d with the data in the hash and return true;
   // else return false;
   bool query(Data& d) const {
      size_t n = bucketNum(d);
      auto it = _buckets[n].begin();
      for (; it != _buckets[n].end(); ++it) {
         if (*it == d) {
            d += (*it).getLoad();
            return true;
         }
      }
      return false;
   }

   // update the entry in hash that is equal to d (i.e. == return true)
   // if found, update that entry with d and return true;
   // else insert d into hash as a new entry and return false;
   bool update(const Data& d) {
      size_t n = bucketNum(d);
      auto it = _buckets[n].begin();
      for (; it != _buckets[n].end(); ++it) {
         if (*it == d) {
            size_t offset = d.getLoad() - (*it).getLoad();
            (*it) += offset;
            return true;
         }
      }
      return false;
   }

   // return true if inserted successfully (i.e. d is not in the hash)
   // return false is d is already in the hash ==> will not insert
   bool insert(const Data& d) {
      if (check(d)) { return false; }
      size_t n = bucketNum(d);
      _buckets[n].push_back(d);
      return true;
   }

   // return true if removed successfully (i.e. d is in the hash)
   // return fasle otherwise (i.e. nothing is removed)
   bool remove(const Data& d) {
      size_t n = bucketNum(d);
      auto it = _buckets[n].begin();
      for (; it != _buckets[n].end(); ++it) {
         if (*it == d) {
            _buckets[n].erase(it);
            return true;
         }
      }
      return false;
   }

private:
   // Do not add any extra data member
   size_t            _numBuckets;
   vector<Data>*     _buckets;

   size_t bucketNum(const Data& d) const {
      return (d() % _numBuckets);
   }
};

#endif // MY_HASH_SET_H
