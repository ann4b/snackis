#include "snackis/db/change.hpp"
#include "snackis/db/ctx.hpp"
#include "snackis/db/trans.hpp"

namespace snackis {
namespace db {
  Trans::Trans(Ctx &ctx):
    ctx(ctx)
  {
    super = ctx.trans;
    ctx.trans = this;
  }

  Trans::~Trans() {
    if (!changes.empty()) { rollback(*this); }
    ctx.trans = super;
  }
  
  void log_change(Trans &trans, Change *change) {
    trans.changes.push_back(std::shared_ptr<Change>(change));
  }

  static void clear(Trans &trans) {
    trans.changes.clear();
  }
  
  void commit(Trans &trans, const opt<str> &lbl) {
    if (trans.changes.empty()) { return; }

    Ctx &ctx(trans.ctx);
    Msg msg(MSG_COMMIT);
    set(msg, Msg::SENDER, &ctx);
    set(msg, Msg::CHANGES, trans.changes);
    put(ctx.proc.inbox, msg);
    
    if (lbl) {
      ctx.undo_stack.emplace_back(ctx, *lbl, trans.changes);
    } else {
      clear(trans);
    }
  }
  
  void rollback(Trans &trans) {
    for (auto &c: trans.changes) { c->rollback(); }
    clear(trans);
  }
}}
