/****************************************************************************
  FileName     [ util.h ]
  PackageName  [ util ]
  Synopsis     [ Define the prototypes of the exported utility functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2007-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/
#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "rnGen.h"
#include "myUsage.h"
#include <ctime>

using namespace std;

// Extern global variable defined in util.cpp
extern RandomNumGen  rnGen;
extern MyUsage       myUsage;

// In myString.cpp
extern int myStrNCmp(const string& s1, const string& s2, unsigned n);
extern size_t myStrGetTok(const string& str, string& tok, size_t pos = 0,
                          const char del = ' ');
extern bool myStr2Int(const string& str, int& num);
extern bool isValidVarName(const string& str);

// In myGetChar.cpp
extern char myGetChar(istream&);
extern char myGetChar();

// In util.cpp
extern int listDir(vector<string>&, const string&, const string&);
extern size_t getHashSize(size_t s);

// Other utility template functions
template<class T>
void clearList(T& l)
{
   T tmp;
   l.swap(tmp);
}

template<class T, class D>
void removeData(T& l, const D& d)
{
   size_t des = 0;
   for (size_t i = 0, n = l.size(); i < n; ++i) {
      if (l[i] != d) { // l[i] will be kept, so des should ++
         if (i != des) l[des] = l[i];
         ++des;
      }
      // else l[i] == d; to be removed, so des won't ++
   }
   l.resize(des);
}

// TODO: add this to the final src
template<class T, class D>
void eraseData(T& l, const D& d) {
   for (size_t i = 0, n = l.size(); i < n; ++i) {
      if ( l[i] == d) {
         l[i] = l[l.size()-1];
         l.pop_back();
      }
   }
}

template<class T, class D>
void eraseOneData(T& l, const D& d) {
   for (size_t i = 0, n = l.size(); i < n; ++i) {
      if ( l[i] == d) {
         l[i] = l[l.size()-1];
         l.pop_back();
         break;
      }
   }
}

template<class D>
void erase( vector<D>& l, const size_t i ) {
   l[i] = l[l.size()-1];
   l.pop_back();
}

// replace l[i] with d if l[i] == c
template<class D, class C>
bool replaceOne( vector<D>& l, const D& d, const C& c ) {
   for (size_t i = 0, n = l.size(); i < n; ++i) {
      if ( l[i] == c) {
         l[i] = d;
         return true;
      }
   }
   return false;
}
template<class D>
void removeDuplicate( vector<D>& v ) {
   if ( v.empty() ) return;
   vector<D> v2;
   v2.reserve( v.size() );
   v2.push_back( v[0] );
   if ( v.size() > 1 ) {
      for( size_t i = 1, n = v.size(); i < n; ++i ) {
         if ( v[i] != v[i-1] ) v2.push_back( v[i] );
      }
   }
   v = v2;
}

struct Timer {

	Timer(string n = "Timer"): _name(n), _timer(0), _total(0), _running(false) {}

	void 		start() { _timer = clock(); _running = true; }
	void		pause() 
	{ 	
		if (_running) {
			_total += (clock() - _timer);
			_running = false;
		}
	}
	void		reset() { _total = 0; }
	void		report()
	{
		cout << setw(20) << _name <<  ": " 
			<< fixed << setprecision(5) 
			<< setw(10) << (double)_total/CLOCKS_PER_SEC << " sec" <<endl;
	}
   int checkCurrentTime()
   {
      return (int)(clock() - _timer) / CLOCKS_PER_SEC;
   }

	string		_name;
	clock_t 	_timer;
	clock_t 	_total;
	bool		_running;
};

void reportTimers();

#endif // UTIL_H
