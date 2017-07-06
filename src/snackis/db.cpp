#include "snackis/ctx.hpp"
#include "snackis/db.hpp"
#include "snackis/core/diff.hpp"
#include "snackis/core/bool_type.hpp"
#include "snackis/core/int64_type.hpp"
#include "snackis/core/set_type.hpp"
#include "snackis/core/str_type.hpp"
#include "snackis/core/time_type.hpp"
#include "snackis/core/uid_type.hpp"
#include "snackis/crypt/pub_key_type.hpp"

namespace snackis {  
  Db::Db(Ctx &ctx):
    setting_key("key", str_type, &BasicSetting::key),
    setting_val("val", str_type, &BasicSetting::val),
    
    settings(ctx, "settings",
	     {&setting_key}, {&setting_val}),

    invite_to(       "to",        str_type,  &Invite::to),
    invite_posted_at("posted_at", time_type, &Invite::posted_at),

    invites(ctx, "invites",
	    {&invite_to}, {&invite_posted_at}),
    
    peer_id(        "id",         uid_type,            &Peer::id),
    peer_created_at("created_at", time_type,           &Peer::created_at),
    peer_changed_at("changed_at", time_type,           &Peer::changed_at),
    peer_name(      "name",       str_type,            &Peer::name),
    peer_email(     "email",      str_type,            &Peer::email),
    peer_crypt_key( "crypt_key",  crypt::pub_key_type, &Peer::crypt_key),

    peers(ctx, "peers", {&peer_id},
	  {&peer_created_at, &peer_changed_at, &peer_name, &peer_email,
	      &peer_crypt_key}),

    peers_sort(ctx, "peers_sort", {&peer_name, &peer_id}, {}),
    
    feed_id(        "id",         uid_type,     &Feed::id),
    feed_owner_id(  "owner_id",   uid_type,     &Feed::owner_id),
    feed_created_at("created_at", time_type,    &Feed::created_at),
    feed_changed_at("changed_at", time_type,    &Feed::changed_at),
    feed_name(      "name",       str_type,     &Feed::name),
    feed_info(      "into",       str_type,     &Feed::info),
    feed_active(    "active",     bool_type,    &Feed::active),
    feed_visible(   "visible",    bool_type,    &Feed::visible),
    feed_peer_ids(  "peer_ids",   uid_set_type, &Feed::peer_ids),
    
    feeds(ctx, "feeds", {&feed_id},
	  {&feed_owner_id, &feed_created_at, &feed_changed_at, &feed_name,
	      &feed_info, &feed_active, &feed_visible, &feed_peer_ids}),

    feeds_sort(ctx, "feeds_sort", {&feed_created_at, &feed_id}, {}),
		  
    post_id(        "id",         uid_type,     &Post::id),
    post_feed_id(   "feed_id",    uid_type,     &Post::feed_id),
    post_owner_id(  "owner_id",   uid_type,     &Post::owner_id),
    post_created_at("created_at", time_type,    &Post::created_at),
    post_changed_at("changed_at", time_type,    &Post::changed_at),
    post_body(      "body",       str_type,     &Post::body),
    post_peer_ids(  "peer_ids",   uid_set_type, &Post::peer_ids),

    posts(ctx, "posts", {&post_id},
	  {&post_feed_id, &post_owner_id, &post_created_at, &post_changed_at,
	      &post_body, &post_peer_ids}),

    posts_sort(ctx, "posts_sort", {&post_created_at, &post_id}, {}),
    feed_posts(ctx, "feed_posts", {&post_feed_id, &post_created_at, &post_id}, {}),
    
    msg_id(        "id",         uid_type,            &Msg::id),
    msg_type(      "type",       str_type,            &Msg::type),
    msg_from(      "from",       str_type,            &Msg::from),
    msg_to(        "to",         str_type,            &Msg::to),
    msg_from_id(   "from_id",    uid_type,            &Msg::from_id),
    msg_to_id(     "to_id",      uid_type,            &Msg::to_id),
    msg_fetched_at("fetched_at", time_type,           &Msg::fetched_at),
    msg_peer_name( "peer_name",  str_type,            &Msg::peer_name),
    msg_crypt_key( "crypt_key",  crypt::pub_key_type, &Msg::crypt_key),
    msg_feed(      "feed",       feeds.rec_type,      &Msg::feed),
    msg_post(      "post",       posts.rec_type,      &Msg::post),
    msg_project(   "project",    projects.rec_type,   &Msg::project),
    msg_task(      "task",       tasks.rec_type,      &Msg::task),
    msg_queue(     "queue",      queues.rec_type,     &Msg::queue),
    
    inbox(ctx, "inbox", {&msg_id},
	  {&msg_type, &msg_fetched_at, &msg_peer_name, &msg_from, &msg_from_id,
	      &msg_crypt_key, &msg_feed, &msg_post, &msg_project, &msg_task,
	      &msg_queue}),

    inbox_sort(ctx, "inbox_sort", {&msg_fetched_at, &msg_id}, {}),
    
    outbox(ctx, "outbox", {&msg_id},
	   {&msg_type, &msg_from, &msg_from_id, &msg_to, &msg_to_id, &msg_peer_name,
	       &msg_crypt_key, &msg_feed, &msg_post, &msg_project, &msg_task,
	       &msg_queue}),

    project_id(        "id",         uid_type,     &Project::id),
    project_owner_id(  "owner_id",   uid_type,     &Project::owner_id),
    project_created_at("created_at", time_type,    &Project::created_at),
    project_changed_at("changed_at", time_type,    &Project::changed_at),
    project_name(      "name",       str_type,     &Project::name),
    project_info(      "info",       str_type,     &Project::info),
    project_active(    "active",     bool_type,    &Project::active),
    project_peer_ids(  "peer_ids",   uid_set_type, &Project::peer_ids),
    
    projects(ctx, "projects", {&project_id},
	     {&project_owner_id, &project_created_at, &project_changed_at,
		 &project_name, &project_info, &project_active, &project_peer_ids}),

    projects_sort(ctx, "projects_sort", {&project_name, &project_id}, {}),
    
    task_id(        "id",         uid_type,     &Task::id),
    task_project_id("project_id", uid_type,     &Task::project_id),
    task_owner_id(  "owner_id",   uid_type,     &Task::owner_id),
    task_created_at("created_at", time_type,    &Task::created_at),
    task_changed_at("changed_at", time_type,    &Task::changed_at),
    task_name(      "name",       str_type,     &Task::name),
    task_info(      "info",       str_type,     &Task::info),
    task_done(      "done",       bool_type,    &Task::done),
    task_peer_ids(  "peer_ids",   uid_set_type, &Task::peer_ids),
    task_queue_ids( "queue_ids",  uid_set_type, &Task::queue_ids),
    
    tasks(ctx, "tasks", {&task_id},
	  {&task_project_id, &task_owner_id, &task_created_at, &task_changed_at,
	      &task_name, &task_info, &task_done, &task_peer_ids, &task_queue_ids}),

    tasks_sort(ctx, "tasks_sort", {&task_created_at, &task_id}, {}),
    
    queue_id(        "id",         uid_type,     &Queue::id),
    queue_owner_id(  "owner_id",   uid_type,     &Queue::owner_id),
    queue_created_at("created_at", time_type,    &Queue::created_at),
    queue_changed_at("changed_at", time_type,    &Queue::changed_at),
    queue_name(      "name",       str_type,     &Queue::name),
    queue_info(      "info",       str_type,     &Queue::info),
    queue_peer_ids(  "peer_ids",   uid_set_type, &Queue::peer_ids),
    
    queues(ctx, "queues", {&queue_id},
	   {&queue_owner_id, &queue_created_at, &queue_changed_at, &queue_name,
	       &queue_info, &queue_peer_ids}),

    queues_sort(ctx, "queues_sort", {&queue_name, &queue_id}, {}),

    queue_task_id(      "id",       uid_type,  &QueueTask::id),
    queue_task_queue_id("queue_id", uid_type,  &QueueTask::queue_id),
    queue_task_at(      "at",       time_type, &QueueTask::at),
    
    queue_tasks(ctx, "queue_tasks",
		{&queue_task_queue_id, &queue_task_at, &queue_task_id},
		{})
  {
    peers.indexes.insert(&peers_sort);
    inbox.indexes.insert(&inbox_sort);
    feeds.indexes.insert(&feeds_sort);
    posts.indexes.insert(&posts_sort);
    posts.indexes.insert(&feed_posts);
    projects.indexes.insert(&projects_sort);
    tasks.indexes.insert(&tasks_sort);
    queues.indexes.insert(&queues_sort);

    peers.on_update.push_back([&](auto &prev_rec, auto &curr_rec) {
	db::set(curr_rec, peer_changed_at, now());
      });

    feeds.on_update.push_back([&](auto &prev_rec, auto &curr_rec) {
	db::set(curr_rec, feed_changed_at, now());
      });

    posts.on_update.push_back([&](auto &prev_rec, auto &curr_rec) {
	db::set(curr_rec, post_changed_at, now());
      });

    projects.on_update.push_back([&](auto &prev_rec, auto &curr_rec) {
	Project prev(ctx, prev_rec), curr(ctx, curr_rec);

	if (curr.peer_ids != prev.peer_ids) {
	  auto feed(find_feed_id(ctx, curr.id));

	  if (feed) {
	    feed->peer_ids = curr.peer_ids;
	    db::update(feeds, *feed);
	  }
	}

	db::set(curr_rec, project_changed_at, now());
      });

    tasks.on_insert.push_back([&](auto &rec) {
	Task tsk(ctx, rec);
	
	for (auto &id: tsk.queue_ids) {
	  auto q(find_queue_id(ctx, id));
	  if (q) { db::insert(ctx.db.queue_tasks, QueueTask(*q, tsk)); }
	}
      });
      
    tasks.on_update.push_back([&](auto &prev_rec, auto &curr_rec) {
	Task prev(ctx, prev_rec), curr(ctx, curr_rec);

	if (curr.peer_ids != prev.peer_ids) {		    	
	  auto feed(find_feed_id(ctx, curr.id));

	  if (feed) {
	    feed->peer_ids = curr.peer_ids;
	    db::update(feeds, *feed);
	  }
	}

	db::set(curr_rec, task_changed_at, now());
	auto queues(diff(prev.queue_ids, curr.queue_ids));
	
	for (auto &id: queues.first) {
	  auto q(find_queue_id(ctx, id));
	  if (q) { db::insert(ctx.db.queue_tasks, QueueTask(*q, prev)); }
	}

	for (auto &id: queues.second) {
	  auto q(find_queue_id(ctx, id));
	  if (q) { db::erase(ctx.db.queue_tasks, QueueTask(*q, prev)); }
	}
      });

    queues.on_update.push_back([&](auto &prev_rec, auto &curr_rec) {
	Queue prev(ctx, prev_rec), curr(ctx, curr_rec);

	if (curr.peer_ids != prev.peer_ids) {		    	
	  auto feed(find_feed_id(ctx, curr.id));

	  if (feed) {
	    feed->peer_ids = curr.peer_ids;
	    db::update(feeds, *feed);
	  }
	}

	db::set(curr_rec, queue_changed_at, now());
      });
  }
}
