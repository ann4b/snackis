#include <iostream>
#include "snackis/ctx.hpp"
#include "snackis/peer.hpp"
#include "snackis/post.hpp"

namespace snackis {
  Post::Post(Ctx &ctx): IdRec(ctx), at(now()), by_id(whoami(ctx).id)
  { }

  Post::Post(Ctx &ctx, const db::Rec<Post> &rec): IdRec(ctx, null_uid) {
    copy(*this, rec);
  }

  Post::Post(const Msg &msg):
    IdRec(msg.ctx, msg.post_id),
    feed_id(msg.feed_id), 
    at(msg.post_at), 
    by_id(msg.from_id),
    body(msg.post_body),
    peer_ids({msg.from_id})
  { }

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
      msg.feed_id = fed.id;
      msg.feed_name = fed.name;
      msg.feed_info = fed.info;
      msg.feed_visible = fed.visible;
      msg.post_id = pst.id;
      msg.post_at = pst.at;
      msg.post_body = pst.body;
      insert(ctx.db.outbox, msg);
    }
  }

  void set_feed(Post &pst, Feed &fed) {
    pst.feed_id = fed.id;
    pst.peer_ids = fed.peer_ids;
  }
}
