CC = gcc
CXX = g++
AR = ar

# This may change
DEFINES += -DNDEBUG
INCLUDES += -I../Include -I../abscommon/abscommon -I/usr/include/postgresql
TARGETDIR = ../Lib/x64/Release
TARGET = $(TARGETDIR)/libabdb.a
SOURDEDIR = ../abdb/abdb
OBJDIR = obj/x64/Release/abdb
SRC_FILES = \
	$(SOURDEDIR)/Database.cpp \
	$(SOURDEDIR)/DatabaseMysql.cpp \
	$(SOURDEDIR)/DatabasePgsql.cpp \
	$(SOURDEDIR)/DatabaseSqlite.cpp \
	$(SOURDEDIR)/stdafx.cpp \
# End changes

CXXFLAGS += $(DEFINES) $(INCLUDES) -MMD -O3 -Wall -Wextra -std=c++14
LINKCMD = $(AR) -rcs "$@"

OBJ_FILES := $(patsubst $(SOURDEDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC_FILES))
$(info $(OBJ_FILES))

$(TARGET): $(OBJ_FILES)
	$(LINKCMD) $(OBJ_FILES)

$(OBJDIR)/%.o: $(SOURDEDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)
$(TARGETDIR):
	mkdir -p $(TARGETDIR)

-include $(OBJ_FILERS:.o=.d)

.PHONY: clean
clean:
	rm -f $(OBJ_FILES) $(TARGET)