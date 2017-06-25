#include <cassert>
#include "snackis/ctx.hpp"
#include "snackis/gui/gui.hpp"
#include "snackis/gui/feed_view.hpp"
#include "snackis/gui/project_view.hpp"

namespace snackis {
namespace gui {
  static void on_cancel(gpointer *_, ProjectView *v) {
    log(v->ctx, "Cancelled project");
    pop_view(*v);
  }

  static void on_save(gpointer *_, ProjectView *v) {
    Ctx &ctx(v->ctx);
    db::Trans trans(ctx);
    
    v->project.name = gtk_entry_get_text(GTK_ENTRY(v->name));
    v->project.info = get_str(GTK_TEXT_VIEW(v->info));
    db::upsert(ctx.db.projects, v->project);
    db::commit(trans);
    log(v->ctx, "Saved project");
    pop_view(*v);
  }

  ProjectView::ProjectView(const Project &project):
    View(project.ctx, "Project", to_str(project.id)),
    project(project),
    name(gtk_entry_new()),
    info(new_text_view()),
    save(gtk_button_new_with_mnemonic("_Save Project")),
    cancel(gtk_button_new_with_mnemonic("_Cancel")) {
    GtkWidget *lbl;

    GtkWidget *frm = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(panel), frm, true, true, 0);

    lbl = gtk_label_new("Name");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(frm), lbl);
    gtk_widget_set_hexpand(name, true);    
    gtk_container_add(GTK_CONTAINER(frm), name);
    gtk_entry_set_text(GTK_ENTRY(name), project.name.c_str());
    
    lbl = gtk_label_new("Info");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(frm), lbl);
    gtk_container_add(GTK_CONTAINER(frm), gtk_widget_get_parent(info));
    set_str(GTK_TEXT_VIEW(info), project.info);
    
    GtkWidget *btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(btns, GTK_ALIGN_END);
    gtk_widget_set_valign(btns, GTK_ALIGN_END);
    gtk_widget_set_margin_top(btns, 10);
    gtk_container_add(GTK_CONTAINER(panel), btns);
        
    g_signal_connect(save, "clicked", G_CALLBACK(on_save), this);
    gtk_widget_set_sensitive(save, project.owner_id == whoami(ctx).id);
    gtk_container_add(GTK_CONTAINER(btns), save);

    g_signal_connect(cancel, "clicked", G_CALLBACK(on_cancel), this);
    gtk_container_add(GTK_CONTAINER(btns), cancel);
    focused = name;
  }
}}
