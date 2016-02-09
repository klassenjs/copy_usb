// generated by Fast Light User Interface Designer (fluid) version 1.0302

#include "UserInterface.h"
#include <cstdio>
#include <unistd.h>

#include <string.h>
#include <stdlib.h>

void UserInterface::cb_m_btn_quit_i(Fl_Button*, void*) {
  m_window->hide();
}
void UserInterface::cb_m_btn_quit(Fl_Button* o, void* v) {
  ((UserInterface*)(o->parent()->user_data()))->cb_m_btn_quit_i(o,v);
}

void UserInterface::cb_watch_fd(int fd, void* v) {
  Fl_Progress *p = (Fl_Progress*)(v);
  char buf[30];
  float f;
  char *pos = buf;
  char *end = pos + 29;

  while ((read(fd, pos, 1) > 0) && (*pos != '\n') && (pos != end)) {
    pos++;
  }

  if (buf[0] == 'E') {
    Fl::remove_fd(fd);
    Fl::delete_widget(p);
    return;
  }

  sscanf(buf, "%f\n", &f);

  p->value(f);
}

UserInterface::UserInterface(int argc, char** argv) {
  { m_window = new Fl_Double_Window(440, 415, "Copy USB");
    m_window->user_data((void*)(this));
    { m_pack = new Fl_Pack(0, 20, 440, 360, "Active Copies");

      m_pack->end();
    } // Fl_Pack* m_pack
    { m_btn_quit = new Fl_Button(380, 385, 55, 25, "Quit");
      m_btn_quit->callback((Fl_Callback*)cb_m_btn_quit);
    } // Fl_Button* m_btn_quit
    m_window->end();
  } // Fl_Double_Window* m_window

  m_window->show(argc, argv);
}

Fl_Progress* UserInterface::add_progress_bar(const char *name)
{
  Fl_Progress *o;
  m_pack->begin();
  o = new Fl_Progress(0, 20, 440, 25, name);
  o->minimum(0);
  o->maximum(100);
  m_pack->end();
  return o;
}

void UserInterface::watch_fd(const char *name, int fd)
{
  // Memory leak, but needed, Fl_Progress doesn't make a copy
  char *n = (char*)malloc(strlen(name) + 1);
  memcpy(n, name, strlen(name)+1);
  Fl_Progress *p = add_progress_bar(n);
  Fl::add_fd(fd, FL_READ, cb_watch_fd, (void*)p);
}
