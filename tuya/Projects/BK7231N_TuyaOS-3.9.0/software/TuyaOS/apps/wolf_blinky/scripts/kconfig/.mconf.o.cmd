cmd_scripts/kconfig/mconf.o := gcc -Wp,-MMD,scripts/kconfig/.mconf.o.d     -D_GNU_SOURCE -I/usr/include/ncursesw -c -o scripts/kconfig/mconf.o scripts/kconfig/mconf.c

source_scripts/kconfig/mconf.o := scripts/kconfig/mconf.c

deps_scripts/kconfig/mconf.o := \
  scripts/kconfig/lkc.h \
    $(wildcard scripts/include/config/prefix) \
  scripts/kconfig/expr.h \
  scripts/kconfig/list.h \
  scripts/kconfig/lkc_proto.h \
  scripts/kconfig/lxdialog/dialog.h \
  /usr/include/ncursesw/ncurses.h \
  /usr/include/ncursesw/ncurses_dll.h \
  /usr/include/ncursesw/unctrl.h \
  /usr/include/ncursesw/curses.h \

scripts/kconfig/mconf.o: $(deps_scripts/kconfig/mconf.o)

$(deps_scripts/kconfig/mconf.o):
