//
// Created by iskakoff on 21/08/16.
//

#ifndef HUBBARD_NSYMMETRY_H
#define HUBBARD_NSYMMETRY_H


#include <queue>
#include "Symmetry.h"
#include "Combination.h"

class NSymmetry: public Symmetry {
public:
  class Sector {
  public:
    friend class NSymmetry;
    friend std::ostream& operator << (std::ostream& o, const NSymmetry::Sector& c) { return o << " (nup+ndown: " <<c._n<<") size: "<<c._size;  }
    Sector(int n, size_t size) : _n(n), _size(size)  {};
  protected:
    bool operator<(Sector s) {
      return _size< s._size;
    }

  public:
    int n() const {return _n;}

    size_t size() const {return _size;}

    void print() const {
      std::cout<<_n;
    }

  private:
    int _n;
    size_t _size;
  };

  NSymmetry(EDParams &p) : Symmetry(p), _N(2*int(p["NSITES"])), _totstate(_N, 0.0), _current_sector(-1, 0),
                           _comb(_N) {
    if(p.exists("arpack.SECTOR") && bool(p["arpack.SECTOR"])) {
      std::vector<std::vector<int> > sectors;
      std::string input = p["INPUT_FILE"];
      alps::hdf5::archive input_file(input, "r");
      input_file>>alps::make_pvp("sectors/values", sectors);
      input_file.close();
      for(auto& sector : sectors) {
        _sectors.push(NSymmetry::Sector(sector[0], (size_t) (_comb.c_n_k(_N, sector[0]))));
      }
    } else {
      for(int i = 0; i<=_N;++i) {
        _sectors.push(NSymmetry::Sector(i, (size_t) (_comb.c_n_k(_N, i))));
      }
    }
  }

  virtual bool next_state() override {
    long long res = 0;
    if(_ind>=_current_sector.size()) {
      return false;
    }
    res=next_basis(_N, _current_sector.n(), _totstate);
    _ind++;
    state() = res;
    return true;
  }

  virtual int index(long long st) override {
    return  _comb.c_n_k(_N, _current_sector.n()) - num(st, _N, _current_sector.n()) - 1;
  }

  virtual void reset() override {
    state() = 0ll;
    _first = true;
    _ind = 0;
  }

  virtual void init() override {
    reset();
    initState(_current_sector.n(), _totstate);
  }

  virtual bool next_sector() override {
    if(_sectors.empty())
      return false;
    _current_sector = _sectors.front();
    _sectors.pop();
    std::cout<<"Diagonalizating N-symmetry sector with total number of particles: "<<_current_sector.n()<<" size: "<<_current_sector.size()<<std::endl;
    return true;
  }

private:

  int _N;
  bool _first;
  int _ind;
  NSymmetry::Sector _current_sector;
  std::queue<NSymmetry::Sector> _sectors;
  std::vector<int> _totstate;
  Combination _comb;

  int next_basis(int n, int k, std::vector < int > &old){
    int res = 0;
    if(_first) {
      initState(k, old);
      _first = false;
      //pass
    }
    else {
      next_combination(n, k, old);
    }
    for (int i = 0; i < k; i++) {
      res += (1 << old[i]);
    }
    return res;
  }
  /**
  *
  */
  void initState(int ik, std::vector<int>& vec) {
    for(int i=0;i<ik;i++) {
      vec[i] = i;
    }
  }

  /**
   *
   */
  bool next_combination(int n, int k, std::vector < int > &old) {
    for (int i = k - 1; i >= 0; i--) {
      if (old[i] < (n - 1 - k + (i + 1))) {
        old[i] += 1;
        for (int j = i + 1; j < k; j++) {
          old[j] = old[j - 1] + 1;
        }
        return true;
      }
    }
    return false;
  }

  inline const int num(long long b, int n, int m) const {
    int res = 0;
    if (((b & (1ll << (_N - n))) == 0) and ((n - 1) > 0) and (m > 0) and (m < n))
      res = num(b, n - 1, m);
    else if (((n - 1) > 0) and (m > 0) and (m < n))
      res = _comb.c_n_k(n - 1, m) + num(b, n - 1, m - 1);
    return res;
  }
};


#endif //HUBBARD_NSYMMETRY_H