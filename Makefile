################################################################################
#                Makefile for projects and dependencies management             #
################################################################################
# this makefiles is supposed to make dependency management a little bit easier
# by providing som structure to projects. It consist of two main components.
# dependencies and projects. dependencies are code repositories needed for your 
# project to compile and are pulled from vcs (for example git). Projects are 
# your local programs. all dependencies are pulled and built in the 
# DEPS_DIR_RELATIVE directory. If these dependencies are librarries that need to 
# be linked, these librarries binaries go to INC_DIR_RELATIVE directory.
#
# See dependencies.mk file to see how to set up a dependency.
# For projects, this makefile assumes they are all built the same. See targets.
#
# Targets:
# 	all - build all dependencies and projects 
# 	projects - build only projects by executing "make" in their respective 
# 			   directories
#   test - build tests for your projects by executing "make test" in their 
#   	   respective directories
#   clean - clean your projects by executing "make clean" in their respective
#           directories
#   purge - same as clean but also removes ALL DEPENDENCIES AND INC_DIR_RELATIVE
#	deps - builds only dependencies
#
#	By default, the makefile lets all information from compiler / makefiles
#	print loudly. To disable most information displayed (may not be posible 
#	with some dependencies) do "make <target> S=y". S stands for "silent"

DEPS_DIR_RELATIVE=dependencies
INC_DIR_RELATIVE=include

###### DONT CHANGE ANYTHING BELOW HERE UNLESS YOU KNOW WHAT YOU ARE DOING ######

DEPS_DIR=$(TOPDIR)/$(DEPS_DIR_RELATIVE)
INC_DIR=$(TOPDIR)/$(INC_DIR_RELATIVE)
TOPDIR=$(PWD)

MAKE_FLAGS:= 
MAKE_CMD_TAIL:=

COLOUR_RED=\033[0;31m
COLOUR_GREEN=\033[0;32m
COLOUR_RESET =\033[0m

ifeq ($(S),y)
	MAKE_FLAGS += -s
	MAKE_CMD_TAIL += > /dev/null
endif

# Include dependencies configuration
include dependencies.mk
DEPENDENCIES:=$(shell grep -E '^[^.# ][^: ]*:' dependencies.mk | cut -d ':' -f 1 | sort)

.PHONY: all $(PROJECTS) test deps
.DEFAULT_GOAL := all

PROJECTS := $(shell ls -d */ | grep -v -e $(DEPS_DIR_RELATIVE) -e $(INC_DIR_RELATIVE))

all: -setup deps projects

projects:
	@for dir in $(PROJECTS); do \
        echo -e "$(COLOUR_GREEN)[project] $${dir}build$(COLOUR_RESET)"; \
        $(MAKE) $(MAKE_FLAGS) INC_DIR=$(INC_DIR) -C $$dir $(MAKE_CMD_TAIL) || { echo -e "$(COLOUR_RED)[project] $$dir FAILED!$(COLOUR_RESET)"; exit 1; }; \
    done

test:
	@for dir in $(PROJECTS); do \
        $(MAKE) $(MAKE_FLAGS) -C $$dir test; \
    done

clean:
	@for dir in $(PROJECTS); do \
        echo -e "$(COLOUR_GREEN)[project] $${dir}clean$(COLOUR_RESET)"; \
        $(MAKE) $(MAKE_FLAGS) -C $$dir clean; \
    done

purge: clean
	@echo -e "$(COLOUR_GREEN)[dependency] clean $(COLOUR_RESET)"
	@rm -rf $(DEPS_DIR)
	@rm -rf $(INC_DIR)

deps:
	@for dep in $(DEPENDENCIES); do \
        if [ -d "$(DEPS_DIR)/$$dep" ]; then \
            echo "Directory $(DEPS_DIR)/$$dep exists. Skipping."; \
        else \
            echo -e "$(COLOUR_GREEN)[dependency] $$dep/build $(COLOUR_RESET)"; \
            $(MAKE) $(MAKE_FLAGS) $$dep DEPS_DIR=$(DEPS_DIR) $(MAKE_CMD_TAIL) || { echo -e "$(COLOUR_RED)[dependency] $$dep FAILED!$(COLOUR_RESET)"; exit 1; }; \
        fi; \
    done

-setup:
	@mkdir -p $(INC_DIR)
	@mkdir -p $(DEPS_DIR)
