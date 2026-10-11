CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -pthread

# Query Qt6 via pkg-config (pkgconf)
QT_CFLAGS := $(shell pkgconf --cflags Qt6Widgets 2>/dev/null || pkg-config --cflags Qt6Widgets)
QT_LIBS := $(shell pkgconf --libs Qt6Widgets 2>/dev/null || pkg-config --libs Qt6Widgets)
QT_LIBEXEC := $(shell pkgconf --variable=libexecdir Qt6Core 2>/dev/null || pkg-config --variable=libexecdir Qt6Core)

# Find moc (either in PATH or in Qt's libexec)
MOC := $(shell which moc 2>/dev/null || echo "$(QT_LIBEXEC)/moc")

TARGET = surfturnbinds
SRCS = main.cpp MainWindow.cpp TurnEngine.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(QT_LIBS)

main.o: main.cpp MainWindow.hpp
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -c $< -o $@

MainWindow.o: MainWindow.cpp MainWindow.hpp moc_MainWindow.cpp
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -c $< -o $@

TurnEngine.o: TurnEngine.cpp TurnEngine.hpp moc_TurnEngine.cpp
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -c $< -o $@

moc_MainWindow.cpp: MainWindow.hpp
	$(MOC) $(QT_CFLAGS) $< -o $@

moc_TurnEngine.cpp: TurnEngine.hpp
	$(MOC) $(QT_CFLAGS) $< -o $@

clean:
	rm -f $(TARGET) *.o moc_*.cpp

.PHONY: all clean
