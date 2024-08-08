cmd_scripts/kconfig/lxdialog/inputbox.o := gcc -Wp,-MMD,scripts/kconfig/lxdialog/.inputbox.o.d     -D_GNU_SOURCE -I/usr/include/ncursesw -c -o scripts/kconfig/lxdialog/inputbox.o scripts/kconfig/lxdialog/inputbox.c

source_scripts/kconfig/lxdialog/inputbox.o := scripts/kconfig/lxdialog/inputbox.c

deps_scripts/kconfig/lxdialog/inputbox.o := \
  scripts/kconfig/lxdialog/dialog.h \
  /usr/include/ncursesw/ncurses.h \
  /usr/include/ncursesw/ncurses_dll.h \
  /usr/include/ncursesw/unctrl.h \
  /usr/include/ncursesw/curses.h \

scripts/kconfig/lxdialog/inputbox.o: $(deps_scripts/kconfig/lxdialog/inputbox.o)

$(deps_scripts/kconfig/lxdialog/inputbox.o):
