/*  Copyright 2011 W.Boeke

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

template<class T>
struct SLList_elem {    // single-linked list element
  T d;
  SLList_elem<T>* nxt;
  SLList_elem(T& d1):d(d1),nxt(0) { }
  ~SLList_elem() { delete nxt; }
};

template<class T>     // single-linked list
struct SLinkedList {
  SLList_elem<T> *lis;
  SLinkedList() { lis=0; }
  ~SLinkedList() { delete lis; }
  void reset() { delete lis; lis=0; }
  void insert(T elm) {
    SLList_elem<T> *p;
    if (!lis)
      lis=new SLList_elem<T>(elm);
    else {
      for (p=lis;;p=p->nxt) {
        if (p->d==elm) { p->d=elm; break; } // assignment maybe needed because of definition of '='
        if (!p->nxt) {
          p->nxt=new SLList_elem<T>(elm);
          break;
        }
      }
    }
  }
  void remove(T elm) {
    SLList_elem<T> *p,*prev;
    if (!lis) return;
    if (lis->d==elm) {
      p=lis->nxt; lis->nxt=0; delete lis; lis=p;
      return;
    }
    for (prev=lis,p=lis->nxt;p;) {
      if (p->d==elm) {
        prev->nxt=p->nxt; p->nxt=0; delete p;
        return;
      }
      else { prev=p; p=p->nxt; }
    }
    puts("SLL: remove: elm not found");
  }
  SLList_elem<T> *remove(SLList_elem<T> *p) { // returns next element
    SLList_elem<T> *p1,*prev;
    if (!lis) { puts("SLL: lis=0"); return 0; }
    if (lis==p) {
      p1=lis->nxt; lis->nxt=0; delete lis; lis=p1;
      return lis;
    }
    for (prev=lis,p1=lis->nxt;p1;) {
      if (p==p1) {
        prev->nxt=p1->nxt; p1->nxt=0; delete p1;
        return prev->nxt;
      }
      prev=p1; p1=p1->nxt;
    }
    puts("SLL: remove: ptr not found");
    return 0;
  }
};

template <class T,Uint32 dim>
struct Array {
  T buf[dim];
  T& operator[](Uint32 ind) {
    if (ind<dim) return buf[ind];
    alert("Array: index=%d (>=%d)",ind,dim);// if (debug) abort();
    return buf[0];
  }
};

template<class T>
T* re_alloc(T* arr,int& len) {
  T* new_arr=new T[len*2];
  for (int i=0;i<len;++i) new_arr[i]=arr[i];
  delete[] arr;
  len*=2;
  return new_arr;
}
