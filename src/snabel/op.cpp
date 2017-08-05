#include <iostream>

#include "snabel/box.hpp"
#include "snabel/coro.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/func.hpp"
#include "snabel/op.hpp"

namespace snabel {
  Op Op::make_begin() {
    Op op(OP_BEGIN);

    op.compile = [](auto &op, auto &scp, bool optimize, auto &out) mutable {
      if (optimize) { op.run(op, scp); }
      return false;
    };
    
    op.run = [](auto &op, auto &scp) { begin_scope(scp.coro); };
    return op;
  }
  
  Op Op::make_call(Func &fn) {
    Op op(OP_CALL);
    op.info = [&fn](auto &op, auto &scp) { return fn.name; };

    op.compile = [&fn](auto &op, auto &scp, bool optimize, auto &out) mutable {
      auto &s(curr_stack(scp.coro));
      if (!optimize || s.empty()) { return false; }
      auto imp(match(fn, scp.coro));

      if (imp && imp->pure && !imp->args.empty()) {
	auto args(pop_args(*imp, scp.coro));
	(*imp)(scp.coro, args);
	out.push_back(Op::make_drop(args.size()));	
	out.push_back(Op::make_push(pop(scp.coro)));	
	return true;
      } else {
	s.clear();
      }

      return false;
    };
    
    op.run = [&fn](auto &op, auto &scp) {
      auto imp(match(fn, scp.coro));
      
      if (imp) {
	(*imp)(scp.coro);
      } else {
	ERROR(Snabel, fmt("Function not applicable: %0\n%1", 
			  fn.name, curr_stack(scp.coro)));
      }
    };

    return op;
  }

  Op Op::make_drop(size_t cnt) {
    Op op(OP_DROP);
    op.info = [cnt](auto &op, auto &scp) { return fmt_arg(cnt); };
    
    op.compile = [cnt](auto &op, auto &scp, bool optimize, auto &out) mutable {
      if (!optimize) { return false; }
      
      Stack &s(curr_stack(scp.coro));
      for (size_t i(0); i < cnt && !s.empty(); i++) {
	s.pop_back();
      }
      
      auto i(0);      
      while (i < cnt && !out.empty() && out.back().code == OP_PUSH) {
	out.pop_back();
	i++;
      }
      
      if (i == cnt) { return true; }
      
      if (i) {
	out.push_back(Op::make_drop(cnt-i));
	return true;
      }

      return false;
    };

    op.run = [cnt](auto &op, auto &scp) {
      for (size_t i(0); i < cnt; i++) { pop(scp.coro); }
    };
    
    return op;    
  }
  
  Op Op::make_end() {
    Op op(OP_END);

    op.compile = [](auto &op, auto &scp, bool optimize, auto &out) mutable {
      if (optimize) {
	bool clear(curr_stack(scp.coro).empty());
	op.run(op, scp);
	if (clear) { curr_stack(scp.coro).clear(); }
      }
      
      return false;
    };

    op.run = [](auto &op, auto &scp) { end_scope(scp.coro); };
    return op;
  }
  
  Op Op::make_id(const str &txt) {
    Op op(OP_ID);
    op.info = [txt](auto &op, auto &scp) { return txt; };

    op.compile = [txt](auto &op, auto &scp, bool optimize, auto &out) {
	auto fnd(find_env(scp, txt));

	if (!fnd) {
	  ERROR(Snabel, fmt("Unknown identifier: %0", txt));
	  return false;
	}

	if (&fnd->type == &scp.coro.exec.func_type) {
	  Func &fn(*get<Func *>(*fnd));
	  out.push_back(Op::make_call(fn));
	  if (optimize) { curr_stack(scp.coro).clear(); }
	  return true;
	}

	if (optimize && &fnd->type != &scp.coro.exec.undef_type) {
	  push(scp.coro, *fnd);
	}
	
	return false;
    };

    op.run = [txt](auto &op, auto &scp) {
      auto fnd(find_env(scp, txt));

      if (!fnd) {
	ERROR(Snabel, fmt("Unknown identifier: %0", txt));
	return;
      }

      push(scp.coro, *fnd);
    };

    return op;
  }

  Op Op::make_jump(const str &tag, opt<Label> lbl) {
    Op op(OP_JUMP);

    op.info = [tag, lbl](auto &op, auto &scp) {
      return fmt("%0 (%1)", tag, lbl ? to_str(lbl->pc) : "?");
    };

    op.compile = [tag, lbl](auto &op, auto &scp, bool optimize, auto &out) {      
      if (optimize) { curr_stack(scp.coro).clear(); }
      auto fnd(scp.labels.find(tag));
      if (fnd == scp.labels.end()) { return false; }
      if (lbl && fnd->second.pc == lbl->pc) { return false; }
      out.push_back(Op::make_jump(tag, fnd->second));
      return true;
    };

    op.run = [tag, lbl](auto &op, auto &scp) {
      if (lbl) {
	while (scp.coro.scopes.size() > lbl->depth) {
	  scp.coro.scopes.pop_back();
	}

	scp.coro.pc = lbl->pc;
      } else {
	ERROR(Snabel, fmt("Missing label: %0", tag));
      }
    };
    
    return op;
  }
  
  Op Op::make_label(const str &tag) {
    Op op(OP_LABEL);
    op.info = [tag](auto &op, auto &scp) { return tag; };

    int64_t prev_pc(-1);
    op.compile = [tag, prev_pc](auto &op, auto &scp, bool optimize, auto &out)
      mutable {
      Coro &cor(scp.coro);
      auto fnd(scp.labels.find(tag));

      if (fnd == scp.labels.end()) {
	scp.labels.emplace(std::piecewise_construct,
			   std::forward_as_tuple(tag),
			   std::forward_as_tuple(cor.scopes.size(), cor.pc));
	prev_pc = cor.pc;
	out.push_back(op);
	return true;
      }

      if (prev_pc == cor.pc) { return false; }
      
      if (prev_pc == -1 || fnd->second.pc == prev_pc) {
	fnd->second.depth = cor.scopes.size();
	prev_pc = fnd->second.pc = cor.pc;
      } else {
	ERROR(Snabel, fmt("Duplicate label: %0", tag));
      }
      
      out.push_back(op);
      return true;
    };

    return op;
  }
  
  Op Op::make_let(const str &id) {
    Op op(OP_LET);
    op.info = [id](auto &op, auto &scp) { return id; };

    int64_t prev_pc(-1);
    op.compile = [id, prev_pc](auto &op, auto &scp, bool optimize, auto &out)
      mutable {
      auto fnd(find_env(scp, id));
      auto &exe(scp.coro.exec);
      
      if (fnd) {
	if (&fnd->type == &exe.undef_type) {
	  if (get<int64_t>(*fnd) != prev_pc) {
	    ERROR(Snabel, fmt("Duplicate binding: %0", id));
	  }
	} else {
	  ERROR(Snabel, fmt("Duplicate binding: %0\n%1", id, *fnd));
	}
      } else {
	put_env(scp, id, Box(exe.undef_type, scp.coro.pc));
	prev_pc = scp.coro.pc;
      }

      if (optimize) {
	auto &s(curr_stack(scp.coro));
	if (!s.empty()) { s.pop_back(); }
      }

      return false;
    };

    op.run = [id](auto &op, auto &scp) {
      auto &s(curr_stack(scp.coro));
      
      if (s.size() == 1) {
	auto v(s.back());
	s.pop_back();
	put_env(scp, id, v);
      } else {
	ERROR(Snabel, fmt("Malformed binding: %0\n%1", id, s));
      }
    };
    
    return op;
  }

  Op Op::make_push(const Box &it) {
    Op op(OP_PUSH);
    op.info = [it](auto &op, auto &scp) { return fmt_arg(it); };
    
    op.compile = [it](auto &op, auto &scp, bool optimize, auto &out) {
      if (optimize) { op.run(op, scp); }
      return false;
    };
    
    op.run = [it](auto &op, auto &scp) { push(scp.coro, it); };
    return op;
  }

  Op Op::make_reset() {
    Op op(OP_RESET);

    op.compile = [](auto &op, auto &scp, bool optimize, auto &out) {
      if (optimize) { op.run(op, scp); }
      return false;
    };
    
    op.run = [](auto &op, auto &scp) { curr_stack(scp.coro).clear(); };
    return op;
  }

  static str def_info(const Op &op, Scope &scp) { return ""; }

  static bool def_compile(const Op &op, Scope &scp, bool optimize, OpSeq &out) {
    return false;
  }

  static void def_run(const Op &op, Scope &scp)
  { }

  Op::Op(OpCode cod):
    code(cod),
    info(def_info), compile(def_compile), run(def_run)
  { }

  str name(const Op &op) {
    switch (op.code){
    case OP_BEGIN:
      return "Begin";
    case OP_CALL:
      return "Call";
    case OP_DROP:
      return "Drop";
    case OP_END:
      return "End";
    case OP_ID:
      return "Id";
    case OP_JUMP:
      return "Jump";
    case OP_LABEL:
      return "Label";
    case OP_LET:
      return "Let";
    case OP_PUSH:
      return "Push";
    case OP_RESET:
      return "Reset";
    };

    ERROR(Snabel, fmt("Invalid op code: %0", op.code));
    return "n/a";
  }

  
  str info(const Op &op, Scope &scp) {
    return op.info(op, scp);
  }

  bool compile(const Op &op, Scope &scp, bool optimize, OpSeq &out) {
    if (op.compile(op, scp, optimize, out)) { return true; }
    out.push_back(op);
    return false;
  }

  void run(const Op &op, Scope &scp) {
    op.run(op, scp);
  }
}
