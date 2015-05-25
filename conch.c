#include <curses.h>
#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>

#include "blastlist.h"
#include "colors.h"
#include "conchbackend.h"
#include "listview.h"
#include "listview_render.h"

int respond_to_keypresses(WINDOW *window, listview *lv) {
  const int input = wgetch(window);

  switch (input) {
  case '0':
    conch_listview_jump_to_top(lv);
    break;

  case 'j':
    conch_listview_select_next_blast(lv);
    break;

  case 'k':
    conch_listview_select_prev_blast(lv);
    break;

  case 's':
    conch_listview_toggle_stick_to_top(lv);
    break;

  case 'q':
    endwin();
    exit(0);
  }

  return input == ERR;
}

WINDOW *init_screen() {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  refresh();

  if (has_colors())
    conch_init_colors();

  // get initial screen setup while we wait for connections
  WINDOW *window = newwin(0, 0, 0, 0);

  nodelay(window, 1);
  wrefresh(window);

  return window;
}

blastlist *init_blasts(mouthpiece *conn) {
  result_set *result = conch_recent_blasts(conn);
  blastlist *blasts = conch_blastlist_from_result_set(result);
  conch_free_result_set(result);
  return blasts;
}

blastlist *update_blasts(mouthpiece *conn, blastlist *blasts) {
  result_set *result = conch_blasts_after(conn, blasts->id);
  blastlist *newblasts = conch_blastlist_from_result_set(result);
  conch_free_result_set(result);

  return conch_blastlist_join(newblasts, blasts);
}

int main(int argc, char **argv) {
  bool stick_to_top;
  int opt;
  static struct option longopts[] = {
    { "stick-to-top", no_argument, NULL, 's' }, { NULL, 0, NULL, 0 },
  };

  while ((opt = getopt_long(argc, argv, "s", longopts, NULL)) != -1) {
    switch (opt) {
    case 's':
      stick_to_top = true;
      break;
    }
  }
  argc -= optind;
  argv += optind;

  WINDOW *main_window = init_screen();

  // Create new list view and render blank screen
  listview *lv = conch_listview_new(stick_to_top);
  conch_listview_render(main_window, lv);

  // Connect to postgres and fetch blasts
  mouthpiece *conn;

  settings config = {
    .page_size = 42,
  };
  do {
    conn = conch_connect(config);
  } while (conn == NULL);

  blastlist *bl = init_blasts(conn);

  while (1) {
    conch_listview_update(lv, update_blasts(conn, bl));

    int at_top = (lv->head == lv->current_blast);
    if (at_top && lv->stick_to_top) {
      conch_listview_jump_to_top(lv);
    }
    conch_listview_render(main_window, lv);
    respond_to_keypresses(main_window, lv);
  }

  endwin();
  conch_listview_free(lv);
}
