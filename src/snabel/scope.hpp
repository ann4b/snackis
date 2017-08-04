#ifndef SNABEL_SCOPE_HPP
#define SNABEL_SCOPE_HPP

#include <list>
#include <map>

#include "snabel/box.hpp"
#include "snabel/func.hpp"
#include "snabel/type.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Coro;

  struct Label {
    int64_t depth, pc;
    
    Label(int64_t depth, int64_t pc);
  };

  using Env = std::map<str, Box>;
  
  struct Scope {
    Coro &coro;
    std::map<str, Label> labels;
    std::list<Env> envs;
    
    Scope(const Scope &src);
    Scope(Coro &cor);
    const Scope &operator =(const Scope &) = delete;
  };

  Env &curr_env(Scope &scp);
  Env &push_env(Scope &scp);
  void pop_env(Scope &scp);

  Box *find_env(Scope &scp, const str &n);
  Box get_env(Scope &scp, const str &n);
  void put_env(Scope &scp, const str &n, const Box &val);
  bool rem_env(Scope &scp, const str &n);
}

#endif