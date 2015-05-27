#include <curses.h>
#include <locale.h>
#include <stdlib.h>

#include "backend.h"
#include "blastlist.h"
#include "caca-driver.h"
#include "cli.h"
#include "colors.h"
#include "conchview-render.h"
#include "conchview.h"
#include "keys.h"
#include "listview.h"
#include "render.h"

// Approximate time to wait between requests to the database (seconds)
#define DB_POLL_INTERVAL 10

// Maximum time to wait for a keypress (tenths of a second)
#define KEY_DELAY 5

WINDOW *init_screen() {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  refresh();

  conch_init_colors();
  ncurses_init_caca_attrs(&caca_attr[0]);

  // get initial screen setup while we wait for connections
  WINDOW *window = newwin(0, 0, 0, 0);

  // Turn on "half delay" mode, in which getch functions will block for up to n
  // tenths of a second before returning ERR.
  halfdelay(KEY_DELAY);

  wrefresh(window);

  return window;
}

void init_blasts(mouthpiece *conn, blastlist *bl) {
  resultset *result = conch_recent_blasts(conn);
  conch_blastlist_prepend_resultset(bl, result);
  conch_resultset_free(result);
}

void update_new_blasts(mouthpiece *conn, blastlist *bl) {
  resultset *result = conch_blasts_after(conn, bl->head->id);
  conch_blastlist_prepend_resultset(bl, result);
  conch_resultset_free(result);
}

void update_old_blasts(mouthpiece *conn, blastlist *bl) {
  resultset *result = conch_blasts_before(conn, bl->tail->id);
  conch_blastlist_append_resultset(bl, result);
  conch_resultset_free(result);
}

mouthpiece *wait_for_connection(settings *config) {
  mouthpiece *conn;
  do {
    conn = conch_connect(*config);
    sleep(1);
  } while (conn == NULL);

  return conn;
}

int main(int argc, char **argv) {
  WINDOW *win;
  blastlist *bl;
  conch_cli_options opts;
  notifications notifications;
  conchview *cv;
  keypress_result res;
  listview *lv;
  mouthpiece *conn;
  settings config = {
    .page_size = 42,
  };

  win = init_screen();
  opts = conch_parse_command_line_args(argc, argv);

  // Create views
  cv = conch_conchview_new(&opts);
  lv = conch_listview_new(&opts);

  // Render conch while loading
  conch_conchview_render(cv, win);

  // Connect to postgres and fetch initial data
  conn = wait_for_connection(&config);
  conch_notifications_init(&notifications, conn);
  bl = conch_blastlist_new();
  init_blasts(conn, bl);
  conch_listview_update(lv, bl);

  view_type current_view = VIEW_LIST;
  void *current_view_state = (void *)lv;

  while (1) {
    if (conch_notifications_poll(&notifications)) {
      update_new_blasts(conn, bl);
      conch_listview_update(lv, bl);
    }

    render_view(win, current_view, current_view_state);

    if (bl->current->next == NULL) {
      update_old_blasts(conn, bl);
    }

    res = conch_keypress_dispatch(wgetch(win), lv);
    if (res == CONCH_EXIT) {
      break;
    }
  }

  endwin();
  conch_conchview_free(cv);
  conch_listview_free(lv);
}
