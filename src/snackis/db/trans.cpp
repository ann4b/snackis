#include "snackis/db/ctx.hpp"
#include "snackis/db/trans.hpp"

namespace snackis {
namespace db {
  Trans::Trans(Ctx &ctx): Trans(ctx, ctx.trans) { }

  Trans::Trans(Ctx &ctx, Trans *super): ctx(ctx), super(super) {
    ctx.trans = this;
  }

  Trans::~Trans() {
    if (!changes.empty()) {
      rollback(*this);
    }
    
    ctx.trans = super;
  }
  
  void log_change(Trans &trans, const Change *change) {
    trans.changes.push_back(change);
  }

  void commit(Trans &trans) {
    if (trans.super) {
      std::copy(trans.changes.begin(),
		trans.changes.end(),
		std::back_inserter(trans.super->changes));
    } else {
      for (const Change *c: trans.changes) {
	c->commit();
      }
    }

    trans.changes.clear();
  }
  
  void rollback(Trans &trans) {
    for (const Change *c: trans.changes) {
      c->rollback();
    }
    
    trans.changes.clear();
  }
}}
