#ifndef SNACKIS_GUI_FEED_VIEW_HPP
#define SNACKIS_GUI_FEED_VIEW_HPP

#include "snackis/feed.hpp"
#include "snackis/gui/feed_history.hpp"
#include "snackis/gui/peer_list.hpp"
#include "snackis/gui/shared_view.hpp"

namespace snackis {
namespace gui {
  struct FeedView: SharedView<Feed> {
    GtkWidget *new_feed_btn, *find_posts_btn, *new_post_btn, *project_btn, *task_btn,
      *post_btn, *name_fld, *tags_fld, *active_fld, *info_fld;
    PeerList peer_lst;
    FeedHistory post_lst;

    FeedView(const Feed &feed);
    bool save() override;
  };
}}

#endif
