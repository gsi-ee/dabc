#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>
#include <menu.h>

#include "dabc/Manager.h"
#include "dabc/Hierarchy.h"
#include "dabc/Configuration.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define CTRLD  4

const char *choices[] = {"Choice 1",
                         "Choice 2",
                         "Choice 3",
                         "Choice 4",
                         "Exit",
                         0};

dabc::Configuration cfg;
std::string fNodeName;
dabc::Hierarchy hierarchy;


bool ConnectNode(const char* nodename)
{
   if ((nodename==0) || (strlen(nodename)==0)) return false;

   new dabc::Manager("cmd", &cfg);

   // ensure that all submitted events are processed
   dabc::mgr.SyncWorker();

   dabc::mgr.CreatePublisher();

   dabc::mgr()->CreateControl(false);

   fNodeName = std::string("dabc://") + nodename;

   dabc::Command cmd("Ping");
   cmd.SetReceiver(fNodeName);
   cmd.SetTimeout(5.);

   int res = dabc::mgr.GetCommandChannel().Execute(cmd);

   if (res != dabc::cmd_true) {
      printf("FAIL connection to %s\n", fNodeName.c_str());
      fNodeName.clear();
   }

   printf("Connection to %s done\n", fNodeName.c_str());
   return true;
}

bool UpdateHierarchy()
{
   // TODO: hierarchy must be requested from the publisher
   dabc::Command cmd("GetHierarchy");
   cmd.SetInt("version", hierarchy.GetVersion());
   cmd.SetReceiver(fNodeName);
   cmd.SetTimeout(5.);

   if (dabc::mgr.GetCommandChannel().Execute(cmd)!=dabc::cmd_true) {
      printf("Fail to get hierarchy\n");
      return false;
   }

   dabc::Buffer buf = cmd.GetRawData();

   if (!buf.null()) {
      // DOUT0("Get raw data %p %u", buf.SegmentPtr(), buf.GetTotalSize());

      if (hierarchy.UpdateFromBuffer(buf)) {
         DOUT0("Update of hierarchy to version %u done", hierarchy.GetVersion());
      }
   }

   return true;
}

struct LevelItem {
   WINDOW *my_menu_win;
   MENU *my_menu;
   ITEM **my_items;
   unsigned num_items;
   int winx, winy;

   LevelItem() :
      my_menu_win(0),
      my_menu(0),
      my_items(0),
      num_items(0),
      winx(0), winy(0)
   {
   }

   virtual ~LevelItem()
   {
      Delete();
   }

   void Delete()
   {
      Hide();

      if (my_items!=0) {
         for(unsigned n = 0; n < num_items; n++)
            free_item(my_items[n]);
         delete[] my_items;
         my_items = 0;
         num_items = 0;
      }

      if (my_menu!=0) {
         free_menu(my_menu);
         my_menu = 0;
      }

      if (my_menu_win!=0) {
         delwin(my_menu_win);
         my_menu_win = 0;
      }
   }

   void SetXY(int x, int y) { winx = x; winy = y; }

   void CreateWin()
   {
      int max = 10;
      for (unsigned n=0;n<num_items;n++) {
         const char* name = item_name(my_items[n]);
         int len = strlen(name);
         if (len>max) max = len;
      }
      if (max>40) max = 40;

      my_menu_win = newwin(num_items+2, max+6, winy, winx);

      mvprintw(LINES - 4, 0, "Create win at x:%d y:%d", winx, winy);

      // keypad(my_menu_win, TRUE);

      /* Set main window and sub window */
      set_menu_win(my_menu, my_menu_win);
      set_menu_sub(my_menu, derwin(my_menu_win, num_items, max+4, 1, 1));
   }

   void CreateTopLevel()
   {
      Delete();
      num_items = 2;
      my_items = new ITEM* [num_items+1];

      my_items[0] = new_item(fNodeName.c_str(), fNodeName.c_str());
      my_items[1] = new_item("Exit", "Exit");
      my_items[2] = 0;

      my_menu = new_menu(my_items);

      menu_opts_off(my_menu, O_SHOWDESC);

      /* Set menu mark to the string " * " */
      set_menu_mark(my_menu, " * ");

      CreateWin();

      Show();
   }

   void CreateFor(dabc::Hierarchy& h)
   {
      Delete();
      num_items = h.NumChilds()+1;
      my_items = new ITEM* [num_items+1];

      for (unsigned n=0;n<num_items-1; n++) {
         const char* objname = h.GetChild(n).GetName();
         my_items[n] = new_item(objname, objname);
      }

      my_items[num_items-1] = new_item("Exit", "Exit");
      my_items[num_items] = 0;

      my_menu = new_menu(my_items);

      menu_opts_off(my_menu, O_SHOWDESC);

      /* Set menu mark to the string " * " */
      set_menu_mark(my_menu, " * ");

      CreateWin();

      Show();
   }

   void Show()
   {
      box(my_menu_win, 0, 0);
      refresh();

      /* Post the menu */
      post_menu(my_menu);
      wrefresh(my_menu_win);
   }

   void Hide()
   {

      if (my_menu) unpost_menu(my_menu);
      if (my_menu_win) {
         wborder(my_menu_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
         wrefresh(my_menu_win);
      }
   }

   int current_item_index()
   {
      return item_index(current_item(my_menu));
   }

   bool is_current_last()
   {
      return current_item_index() + 1 == (int) num_items;
   }

   const char* curr_name()
   {
      return item_name(current_item(my_menu));
   }
};



int main(int argc, char* argv[])
{
   if (argc<2) {
      printf("No any node specified\n");
      return 7;
   }

   if (!ConnectNode(argv[1])) return 7;

   if (!UpdateHierarchy()) return 7;

   /* Initialize curses */
   initscr();
   start_color();
   cbreak();
   noecho();
   keypad(stdscr, TRUE);
   init_pair(1, COLOR_RED, COLOR_BLACK);

   mvprintw(LINES - 3, 0, "Press Enter to show submenu");
   mvprintw(LINES - 2, 0, "Press F1 to exit");

   LevelItem items[20];

   int level = 0;

   items[level].SetXY(2,2);
   items[level].CreateTopLevel();

   bool doagain = true;

   dabc::Hierarchy curr;

   while(doagain) {
      // int c = wgetch(item.my_menu_win);
      int c = getch();
      switch(c) {
         case KEY_F(1):
            doagain = false;
            break;
         case KEY_DOWN:
            menu_driver(items[level].my_menu, REQ_DOWN_ITEM);
            break;
         case KEY_UP:
            menu_driver(items[level].my_menu, REQ_UP_ITEM);
            break;
         case 10:
            mvprintw(LINES - 4, 0, "Pressed Enter");

            if ((level==0) && items[level].is_current_last()) { doagain = false; break; }

            if (items[level].is_current_last()) {

               items[level].Delete();
               level--;
               items[level].Show();
               if (level==0) curr.Release();
                        else curr = dabc::Hierarchy(curr.GetParent());
            } else {

               if (curr.null()) {
                  curr = hierarchy;
               } else {
                  dabc::Reference child = curr.FindChild(items[level].curr_name());
                  if (child.null()) {
                     mvprintw(LINES - 4, 0, "Child not found");
                     break;
                  }
                  if (child.NumChilds()==0) {
                     mvprintw(LINES - 4, 0, "Child %s has no more levels", child.GetName());
                     break;
                  }

                  curr << child;
               }

               int winx = items[level].winx + 4;
               int winy = items[level].winy + items[level].current_item_index() + 2;

               level++;

               items[level].SetXY(winx, winy);
               items[level].CreateFor(curr);
            }

            break;
         case 27:
            mvprintw(LINES - 4, 0, "Pressed ESC");

            if (level==0) { doagain = false; break; }

            items[level].Delete();
            level--;
            items[level].Show();

            if (level==0) curr.Release();
                     else curr = dabc::Hierarchy(curr.GetParent());

            break;
         default:
            mvprintw(LINES - 4, 0, "Pressed some key %d level %d indx %d", c, level, items[level].current_item_index());
            break;

      }
      wrefresh(items[level].my_menu_win);
   }

   for (int n=level; n>=0;n--)
      items[n].Delete();

   endwin();
}
