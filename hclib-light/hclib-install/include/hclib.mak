ifeq ("$(HCLIB_ROOT)", "")
  $(error Please set teh HCLIB_ROOT environment variable.)
endif

PROJECT_BASE_FLAGS := -I$(HCLIB_ROOT)/include
PROJECT_CFLAGS     := -std=c11 $(PROJECT_BASE_FLAGS)
PROJECT_CXXFLAGS   := -std=c++11 $(PROJECT_BASE_FLAGS)
PROJECT_LDFLAGS    := -L$(HCLIB_ROOT)/lib -L$(ARGOBOTS_INSTALL_DIR)lib
PROJECT_LDLIBS     := -lhclib -Wl,-rpath,$(HCLIB_ROOT)/lib -labt -Wl,-rpath,$(ARGOBOTS_INSTALL_DIR)lib
