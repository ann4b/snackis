#include "snackis/ctx.hpp"
#include "snackis/gui/gui.hpp"
#include "snackis/gui/login.hpp"
#include "snackis/gui/reader.hpp"

namespace snackis {
namespace gui {
  static void on_login(GtkWidget *_, Login *login) {
    const str pass(gtk_entry_get_text(GTK_ENTRY(login->pass)));

    if (login->repeat) {  
      if (pass != gtk_entry_get_text(GTK_ENTRY(login->repeat))) {
	log(login->ctx, "Specified passwords don't match, please try again");
	gtk_widget_grab_focus(login->pass);
	return;
      }
      
      db::init_pass(login->ctx, pass);
    } if (!db::login(login->ctx, pass)) {
	log(login->ctx, "Wrong password, please try again");
	return;
    }

    login->pop_view();
    Ctx &ctx(login->ctx);
    
    open(ctx);
    Peer &me(whoami(ctx));
    if (!me.name.empty()) { log(ctx, fmt("Welcome back, %0", me.name)); }
    
    gui::reader.emplace(ctx);
    gtk_box_pack_start(GTK_BOX(left_panel), ptr(*reader), false, false, 0);
    focus(*gui::reader);
    gtk_widget_show_all(gui::window);
  }
  
  Login::Login(Ctx &ctx):
    View(ctx, "Welcome"),
    pass(gtk_entry_new()),
    repeat(nullptr) {
    GtkWidget *lbl;

    GtkWidget *frm = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(panel), frm, true, true, 0);
    gtk_widget_set_halign(frm, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(frm, GTK_ALIGN_CENTER);
    
    lbl = gtk_label_new("Password");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(frm), lbl, false, false, 5);

    gtk_entry_set_visibility(GTK_ENTRY(pass), false);
    gtk_entry_set_width_chars(GTK_ENTRY(pass), 50);
    g_signal_connect(pass, "activate", G_CALLBACK(on_login), this);
    gtk_box_pack_start(GTK_BOX(frm), pass, false, false, 0);

    if (!pass_exists(ctx)) {
      lbl = gtk_label_new("Repeat");
      gtk_widget_set_halign(lbl, GTK_ALIGN_START);
      gtk_box_pack_start(GTK_BOX(frm), lbl, false, false, 5);
      
      repeat = gtk_entry_new();
      gtk_entry_set_visibility(GTK_ENTRY(repeat), false);
      gtk_entry_set_width_chars(GTK_ENTRY(repeat), 50);
      g_signal_connect(repeat, "activate", G_CALLBACK(on_login), this);
      gtk_box_pack_start(GTK_BOX(frm), repeat, false, false, 0);
    }
    
    btn = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(btn), "Login");
    gtk_box_pack_start(GTK_BOX(frm), btn, false, false, 10);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_login), this);
    gtk_widget_set_halign(btn, GTK_ALIGN_CENTER);
  }

  void Login::focus() {
    gtk_widget_grab_focus(pass);
  }
}}