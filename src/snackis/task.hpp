#ifndef SNACKIS_TASK_HPP
#define SNACKIS_TASK_HPP

#include <set>

#include "snackis/id_rec.hpp"
#include "snackis/core/str.hpp"
#include "snackis/core/time.hpp"
#include "snackis/core/uid.hpp"

namespace snackis {
  struct Task: IdRec {
    UId project_id, owner_id;
    Time created_at, changed_at;
    str name, info;
    std::set<str> tags;
    int64_t prio;
    bool done;
    Time done_at;
    std::set<UId> peer_ids;
    
    Task(Ctx &ctx);
    Task(Ctx &ctx, const db::Rec<Task> &rec);   
    Task(const Msg &msg);
  };

  opt<Task> find_task_id(Ctx &ctx, UId id);
  Task get_task_id(Ctx &ctx, UId id);
  Feed get_feed(const Task &tsk);
  void set_project(Task &tsk, Project &project);
  void send(const Task &tsk);
}

#endif
