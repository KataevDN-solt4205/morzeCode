SRC_DIR   = $(PWD)/src/
INC_DIR   = $(PWD)/include/
BUILD_DIR = $(PWD)/build/
OBJ_DIR   = $(BUILD_DIR)obj/
BIN_DIR   = $(BUILD_DIR)bin/

SRC_CXX   = $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)*.cpp))
OBJ_TMP   = $(foreach odir,$(notdir $(patsubst %.cpp,%.oc,$(SRC_CXX))),$(addprefix $(OBJ_DIR), $(odir)))
INCLUDES  = $(addprefix -I, $(INC_DIR))
LIBS      = -lstdc++ -pthread
CXXFLAGS  = -std=gnu++0x -Wall -Werror
CXX       = gcc
MAKEFLAGS += --no-print-directory

TST_OBJ   = $(OBJ_DIR)main_test.oc
APP_OBJ   = $(OBJ_DIR)main.oc

OBJ       = $(filter-out $(APP_OBJ), $(OBJ_TMP))

.PHONY: all clean checkdirs

TARGETS = clean checkdirs codeMorze test 

all:
	@for t in $(TARGETS); \
	do \
		$(MAKE) $$t ; \
	done

codeMorze: $(OBJ) $(APP_OBJ)
	@printf "  CXX      $(OBJ)\n"
	@$(CXX) $(OBJ) $(APP_OBJ) $(CXXFLAGS) $(INCLUDES) $(LIBS) -o $(BIN_DIR)$@

$(TST_OBJ):
	@$(CXX) $(PWD)/test/test_main.cpp -c $(CXXFLAGS) $(INCLUDES) $(LIBS) -o $(TST_OBJ)

test: $(OBJ) $(TST_OBJ) 
	@$(CXX) $(OBJ) $(TST_OBJ) $(CXXFLAGS) $(INCLUDES) $(LIBS) -o $(BIN_DIR)testMorze

checkdirs: 
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

clean:
	-@rm -r $(BIN_DIR) 2>/dev/null || true
	-@rm -r $(OBJ_DIR) 2>/dev/null || true
	-@rm -r $(BUILD_DIR) 2>/dev/null || true

# DYNAMICALLY BUILT TARGETS
define make-source-cxx
$2: $1
	@printf "  CXX      $$<\n"
	@$(CXX) $$< -c $(CXXFLAGS) $(INCLUDES) $(LIBS) -o $$@	
endef

$(foreach sdir,$(SRC_CXX),$(eval $(call make-source-cxx,$(sdir),$(OBJ_DIR)$(notdir $(patsubst %.cpp,%.oc,$(sdir))))))
