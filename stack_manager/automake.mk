bin_PROGRAMS += \
	stack_manager/stack_manager
#man_MANS += stack_manager/stack_manager.8

stack_manager_stack_manager_SOURCES = \
	stack_manager/stack_manager.c \
	stack_manager/stack_manager.h
stack_manager_stack_manager_LDADD = lib/libopenflow.a $(FAULT_LIBS) $(SSL_LIBS)


#EXTRA_DIST += stack_manager/stack_manager.8.in
#DISTCLEANFILES += stack_manager/stack_manager.8

#include secchan/commands/automake.mk
