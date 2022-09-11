GCC=g++ -std=c++2a
GTK_ARGS=`pkg-config gtkmm-4.0 --cflags --libs`
LIBS=-lGL -lGLEW

BUILDDIR=build
SRCDIR=src
# WILD CARDS FOR COMPILATION
H_SRCS := $(wildcard $(SRCDIR)/*.hpp)
C_SRCS := $(wildcard $(SRCDIR)/*.cpp)
C_OBJS := $(C_SRCS:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

clouds: $(H_SRCS) $(C_OBJS)
	$(GCC) -g -o clouds $(C_OBJS) $(GTK_ARGS) $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(H_SRCS) | $(BUILDDIR)
	$(GCC) -g -c -o $@ $< $(GTK_ARGS)

$(BUILDDIR): 
	mkdir $@

clean:
	rm build/*
	rm clouds
