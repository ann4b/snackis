#include <iostream>
#include "snackis/ctx.hpp"
#include "snackis/peer.hpp"
#include "snackis/post.hpp"

namespace snackis {
  Post::Post(Ctx &ctx):
    IdRec(ctx),
    owner_id(whoami(ctx).id),
    created_at(now()),
    changed_at(created_at)
  { }

  Post::Post(Ctx &ctx, const db::Rec<Post> &rec): IdRec(ctx, null_uid) {
    copy(*this, rec);
  }

  Post::Post(const Msg &msg):
    IdRec(msg.ctx, *db::get(msg.post, msg.ctx.db.post_id))
  {
    db::copy(*this, msg.post);    
    
    for (auto i = peer_ids.begin(); i != peer_ids.end();) { 
      if (find_peer_id(ctx, *i)) { i++; }
      else { peer_ids.erase(i); }
    }

    peer_ids.insert(msg.from_id);
  }

  opt<Post> find_post_id(Ctx &ctx, UId id) {
    db::Rec<Post> rec;
    set(rec, ctx.db.post_id, id);
    if (!load(ctx.db.posts, rec)) { return nullopt; }
    return Post(ctx, rec);
  }

  void send(const Post &pst) {
    Ctx &ctx(pst.ctx);
    Feed fed(get_feed_id(ctx, pst.feed_id));
    
    for (auto &pid: pst.peer_ids) {
      Peer per(get_peer_id(ctx, pid));
      Msg msg(ctx, Msg::POST);
      msg.to = per.email;
      msg.to_id = per.id;
      db::copy(ctx.db.feeds, msg.feed, fed);
      db::copy(ctx.db.posts, msg.post, pst);
      insert(ctx.db.outbox, msg);
    }
  }

  void set_feed(Post &pst, Feed &fed) {
    pst.feed_id = fed.id;
    pst.peer_ids = fed.peer_ids;
  }
}
