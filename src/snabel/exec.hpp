#ifndef SNABEL_EXEC_HPP
#define SNABEL_EXEC_HPP

#include <atomic>
#include <map>

#include "snabel/fiber.hpp"
#include "snabel/macro.hpp"
#include "snabel/sym.hpp"
#include "snabel/type.hpp"
#include "snackis/core/uid.hpp"

namespace snabel {
  struct Exec {
    std::map<str, Macro> macros;
    std::map<UId, Fiber> fibers;
    std::deque<Type> types;
    std::map<str, Func> funcs;

    Fiber &main;
    Type &meta_type, &any_type,
      &bool_type, &func_type, &i64_type, &lambda_type, &str_type,
      &undef_type, &void_type;
    std::atomic<Sym> next_sym;
    
    Exec();
    Exec(const Exec &) = delete;
    const Exec &operator =(const Exec &) = delete;
  };

  Macro &add_macro(Exec &exe, const str &n, Macro::Imp imp);

  Type &add_type(Exec &exe, const str &n);

  FuncImp &add_func(Exec &exe,
		    const str n,
		    const ArgTypes &args,
		    Type &rt,
		    FuncImp::Imp imp);

  Sym gensym(Exec &exe);
}

#endif
