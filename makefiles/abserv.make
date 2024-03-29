include makefile.common

# This may change
INCLUDES += -I../Include/recastnavigation -I../Include/ai -I../Include/DirectXMath -I../absmath/absmath -I../abscommon/abscommon 
TARGETDIR = ../Bin
TARGET = $(TARGETDIR)/abserv$(SUFFIX)
SOURDEDIR = ../abserv/abserv
OBJDIR = obj/x64/$(CONFIG)/abserv
LIBS += -lpthread -luuid -llua5.3 -labscommon -labcrypto -labsmath -lpugixml -ldetour -lstdc++fs
CXXFLAGS += -fexceptions
PCH = $(SOURDEDIR)/stdafx.h
CXXFLAGS += -Werror
# End changes

SRC_FILES = $(filter-out $(SOURDEDIR)/stdafx.cpp, $(wildcard $(SOURDEDIR)/*.cpp $(SOURDEDIR)/*/*.cpp))

CXXFLAGS += $(DEFINES) $(INCLUDES)

OBJ_FILES := $(patsubst $(SOURDEDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC_FILES))
#$(info $(OBJ_FILES))
GCH = $(PCH).gch

all: $(TARGET)

$(TARGET): $(GCH) $(OBJ_FILES)
	@$(MKDIR_P) $(@D)
	$(LINKCMD_EXE) $(OBJ_FILES) $(LIBS)

$(OBJDIR)/%.o: $(SOURDEDIR)/%.cpp
	@$(MKDIR_P) $(@D)
	$(CCACHE) $(CXX) $(CXXFLAGS) -MMD -c $< -o $@

# PCH
$(GCH): $(PCH)
	$(CXX) -x c++-header $(CXXFLAGS) -c $< -o $@

-include $(OBJ_FILES:.o=.d)

.PHONY: clean
clean:
	rm -f $(GCH) $(OBJ_FILES) $(TARGET)
