# Makefile for dependency definitions. Consider the definitions to be a 
# shell script but executed by make.
# You are free to use the following commands. You can also use any variabled 
# from main Makefile that is defined BEFORE include statement.

# GIT_CMD - command to clone a repository from git
# MAKE_CMD - command to execute make in dependency directory

GIT_CMD=git clone
MAKE_CMD=make $(MAKE_FLAGS) -C 

# if the parent Makefile was ran with S=y, silence git.
ifeq ($(S),y)
GIT_CMD += -q
endif

##### DEFINE YOUR DEPENDENCIES HERE #####

CCLog:
	$(GIT_CMD) https://github.com/CrusaderSVK287/CCLog.git $(DEPS_DIR)/$@
	@cd $(DEPS_DIR)/$@ && git checkout -q v1.2.2
	$(MAKE_CMD) $(DEPS_DIR)/$@
	cp $(DEPS_DIR)/$@/bin/libcclog.so $(INC_DIR)/
	cp $(DEPS_DIR)/$@/src/cclog.h $(INC_DIR)/
	cp $(DEPS_DIR)/$@/src/cclog_macros.h $(INC_DIR)/

cJSON:
	$(GIT_CMD) https://github.com/DaveGamble/cJSON.git $(DEPS_DIR)/$@
	@mkdir $(DEPS_DIR)/$@/build
	@cd $(DEPS_DIR)/$@/build && git checkout -q v1.7.17 && cmake ..
	$(MAKE_CMD) $(DEPS_DIR)/$@
	cp $(DEPS_DIR)/$@/libcjson.so* $(INC_DIR)/
	cp $(DEPS_DIR)/$@/cJSON.h $(INC_DIR)/
	cp $(DEPS_DIR)/$@/libcjson_utils.so* $(INC_DIR)/
	cp $(DEPS_DIR)/$@/cJSON_Utils.h $(INC_DIR)/

greatest:
	$(GIT_CMD) https://github.com/silentbicycle/greatest $(DEPS_DIR)/$@
	cp $(DEPS_DIR)/$@/greatest.h $(TOPDIR)/server/test
	cp $(DEPS_DIR)/$@/greatest.h $(TOPDIR)/gui/test
	cp $(DEPS_DIR)/$@/greatest.h $(INC_DIR)
