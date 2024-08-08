cmd_scripts/kconfig/parser.tab.o := gcc -Wp,-MMD,scripts/kconfig/.parser.tab.o.d     -I /home/maaz/GitHub/ezlopi-bkfirmware/tuya/Projects/BK7231N_TuyaOS-3.9.0/software/TuyaOS/apps/wolf_blinky/scripts/kconfig -c -o scripts/kconfig/parser.tab.o scripts/kconfig/parser.tab.c

source_scripts/kconfig/parser.tab.o := scripts/kconfig/parser.tab.c

deps_scripts/kconfig/parser.tab.o := \
  scripts/kconfig/lkc.h \
    $(wildcard scripts/include/config/prefix) \
  scripts/kconfig/expr.h \
  scripts/kconfig/list.h \
  scripts/kconfig/lkc_proto.h \
  scripts/kconfig/internal.h \

scripts/kconfig/parser.tab.o: $(deps_scripts/kconfig/parser.tab.o)

$(deps_scripts/kconfig/parser.tab.o):
