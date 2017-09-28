DEBUG = 1

LOGS = \
	   INFO \
	   ERROR \
	   WARN \
	   CRITICAL \
	   CUSTOM \
	   ALL

NO_LOGS = \
		  EPOLL_OPS \
		  STAT_SERIALIZATION

SRC_DIR = src/
GLC_DIR = $(SRC_DIR)global_controller/
COM_DIR = $(SRC_DIR)common/
TST_DIR = test/

SRC_DIRS = $(GLC_DIR) $(COM_DIR)

TARGET=global_controller
MAIN=$(GLC_DIR)main.c

BLD_DIR = build/
DEP_DIR = $(BLD_DIR)depends/
OBJ_DIR = $(BLD_DIR)objs/
RES_DIR = $(BLD_DIR)reults/
TST_BLD_DIR = $(BLD_DIR)test/

BLD_DIRS = $(BLD_DIR) $(DEP_DIR) $(OBJ_DIR) $(RES_DIR)

CLEANUP=rm -f
CLEAN_DIR=rm -rf
MKDIR=mkdir -p

OPTIM=0

CC:=gcc
CXX:=g++

COMPILE=$(CC) -c
COMPILE_PP=$(CXX) -c
DEPEND=$(CC) -MM -MF
DEPEND_PP=$(CXX) -MM -MF
FINAL=$(CXX)
FINAL_TEST=$(CC)

SELF=./global_controller.mk

LOG_DEFINES=$(foreach logname, $(LOGS), -DLOG_$(logname)) \
			$(foreach logname, $(NO_LOGS), -DNO_LOG_$(logname))

CFLAGS=-Wall -pthread -O$(OPTIM) $(LOG_DEFINES)
CC_EXTRAFLAGS = --std=gnu99

ifeq ($(DEBUG), 1)
  CFLAGS+=-ggdb
endif

RESOURCE_EXTS=.txt .json

TST_DIRS = $(patsubst $(SRC_DIR)%/, $(TST_DIR)%/, $(SRC_DIRS))


TSTS = $(foreach TST_D, $(TST_DIRS), $(wildcard $(TST_D)*.c))
TST_MKS = $(foreach TST_D, $(TST_DIRS), $(wildcard $(TST_D)*.mk))
TST_RSCS = $(foreach TST_D, $(TST_DIRS), $(foreach EXT, $(RESOURCE_EXTS), $(wildcard $(TST_D)*$(EXT))))
TST_OBJS = $(patsubst $(TST_DIR)%.c, $(TST_BLD_DIR)%.o, $(TSTS))

SRCS = $(foreach src_dir, $(SRC_DIRS), $(wildcard $(src_dir)*.c))
SRCS_PP = $(foreach src_dir, $(SRC_DIRS), $(wildcard $(src_dir)*.cc))

TST_BLDS = $(patsubst $(TST_DIR)%.c, $(TST_BLD_DIR)%.out, $(TSTS))
RESULTS = $(patsubst $(TST_DIR)%.c, $(RES_DIR)%.txt, $(TSTS))
TST_BLD_RSCS = $(patsubst $(TST_DIR)%, $(TST_BLD_DIR)%, $(TST_RSCS))

DEP_DIRS = $(patsubst $(SRC_DIR)%/, $(DEP_DIR)%/, $(SRC_DIRS))
DEP_TST = $(patsubst $(TST_DIR)%.c, $(DEP_DIR)%.d, $(TSTS))
DEP_SRC = $(patsubst $(SRC_DIR)%.c, $(DEP_DIR)%.d, $(SRCS))

OBJ_DIRS = $(patsubst $(SRC_DIR)%/, $(OBJ_DIR)%/, $(SRC_DIRS))
OBJECTS = $(patsubst $(SRC_DIR)%.c, $(OBJ_DIR)%.o, $(SRCS)) \
		  $(patsubst $(SRC_DIR)%.cc, $(OBJ_DIR)%.o, $(SRCS_PP))

OBJECTS_NOMAIN = $(patsubst $(SRC_DIR)%.c, $(OBJ_DIR)%.o, $(filter-out $(MAIN), $(SRCS)))

TST_BLD_DIRS = $(patsubst $(SRC_DIR)%/, $(TST_BLD_DIR)%/, $(SRC_DIRS))
RES_DIRS = $(patsubst $(SRC_DIR)%/, $(RES_DIR)%/, $(SRC_DIRS))

INCS=$(GLC_DIR) $(COM_DIR)

CFLAGS+= $(foreach inc, $(INCS), -I$(inc))

define test_dep_name
$(notdir $(subst Test_,,$1))_DEPS
endef

define test_filters
$(subst Test_,, $(patsubst $(TST_BLD_DIR)%, $(OBJ_DIR)%.o, $1)) \
	$(foreach dep, $($(call test_dep_name, $1)), $(OBJ_DIR)$(dep:.c=.o))
endef

CCFLAGS=$(CFLAGS) $(CC_EXTRAFLAGS)
CPPFLAGS=$(CFLAGS) $(CPP_EXTRAFLAGS)

TEST_CFLAGS= $(CCFLAGS) -I$(TST_DIR) -lcheck_pic -lrt -lc -lpcap -lm

DIRS=$(BLD_DIRS) $(OBJ_DIRS) $(DEP_DIRS) $(TST_BLD_DIRS) $(RES_DIRS)

all: dirs ${TARGET}

dirs: $(DIRS)
	echo $(TST_OBJS)

depends: $(DEP_DIRS) ${DEP_SRC}

$(TARGET): ${OBJECTS} ${LEG_OBJ}
	$(FINAL) -o $@ $^ $(CFLAGS)

test: all $(TST_BLD_RSCS) test-results
	echo $(TSTS)

test-blds: $(TST_OBJS) $(TST_BLDS) $(TST_BLD_RSCS)

test-results: all test-blds $(RESULTS)
	@echo "-----------------------\nTEST OUTPUT:\n-----------------------"
	@for FILE in $(filter-out all test-blds, $^); do \
		if grep -q ":[FE]:" "$$FILE"; then \
			echo ___ $$FILE ___ ; \
			cat $$FILE; \
		else \
			echo ___ $$FILE ___ ; \
			tail -1 $$FILE; \
		fi \
	done
	@echo "-----------------------\nFAILURES:\n-----------------------"
	@-grep -s ":F:" $^; echo "";
	@echo "-----------------------\nERRORS:\n-----------------------"
	@-grep -s ":E:" $^; echo "";
	@echo "\nDONE"
	@if grep -q ":[FE]:" $(filter-out all test-blds, $^); then \
		false;\
	fi

# Output the results of the tests by executing each of the builds
# of the tests. Output STDOUT and STDERR to the name of the rule
$(RES_DIR)%.txt: $(TST_BLD_DIR)%.out
	-./$< > $@ 2>&1

$(TST_BLD_DIR)%.o:: $(TST_DIR)%.c $(SELF)
	$(COMPILE) $(TEST_CFLAGS) $< -o $@

# creates the test executables by linking the test objects with the build objects excluding 
# the specific source under test
$(TST_BLD_DIR)%.out:: $(TST_BLD_DIR)%.o $(OBJECTS_NOMAIN) $(LEG_OBJ)
	$(FINAL_TEST) -o $@ $(filter-out $(call test_filters, $(@:.out=)), $^) $(TEST_CFLAGS)

$(TST_BLD_DIR)%: $(TST_DIR)%
	@cp $^ $@

# Creates object files from the source file
$(OBJ_DIR)%.o:: $(SRC_DIR)%.c $(SELF)
	$(COMPILE) $(CCFLAGS) $< -o $@

$(OBJ_DIR)%.o:: $(SRC_DIR)%.cc $(SELF)
	$(COMPILE_PP) $(CPPFLAGS) $< -o $@

$(DEP_SRC): $(DEP_DIRS)

$(DEP_DIR)%.d:: $(SRC_DIR)%.c $(LEG_OBJ)
	@$(DEPEND) $@ -MT $(patsubst $(DEP_DIR)%.d, $(OBJ_DIR)%.o, $@) $(TEST_CFLAGS) $<

$(DEP_DIR)%.d:: $(TST_DIR)%.c $(LEG_OBJ)
	@$(DEPEND) $@ -MT $(patsubst $(DEP_DIR)%.d, $(TST_BLD_DIR)%.o, $@) $(TEST_CFLAGS) $<


$(DIRS):
	@$(MKDIR) $@

clean:
	$(CLEAN_DIR) $(BLD_DIR)
	$(foreach leg_lib, $(LEG_LIBS), $(CLEAN_DIR) $(PATH_SRC_LEG)$(leg_lib)/build)
	$(CLEANUP) $(TARGET)

ifneq ($(MAKECMDGOALS), clean)
-include $(DEP_TST)
-include $(DEP_SRC)
-include $(TST_MKS)
endif

.PHONY: all
.PHONY: clean
.PHONY: test
.PHONY: legacy
.PHONY: depends
.PRECIOUS: $(DEP_DIR)%.d $(RES_DIR)%.txt $(OBJ_DIR)%.o $(TST_BLD_DIR)%.out
