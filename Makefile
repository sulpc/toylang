CC          := i686-w64-mingw32-gcc
SUBDIRS     := basic vm complier utils utils/heap utils/queue
OBJDIR      := objs
TARGET      := toylang
CFLAGS      := $(addprefix -I,$(SUBDIRS)) -Wall -g -DHOST_DEBUG=1
LDFLAGS     := -g

CFILES      := $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.c))
CFILEBASES  := $(notdir $(CFILES))
DEPENDS     := $(addprefix $(OBJDIR)/,$(CFILEBASES:.c=.d))
OFILES      := $(addprefix $(OBJDIR)/,$(CFILEBASES:.c=.o))

all: $(TARGET)
	@echo done!

$(TARGET): $(OFILES)
	$(CC) $(LDFLAGS) $(OBJDIR)/*.o -o $(TARGET)

# include all *.d file
sinclude $(DEPENDS)

# rules to gen *.d file
define rule4dfile
$(2): $(1)
	@echo gen depend file of $(1)
	@$(CC) -MM $(CFLAGS) $(1) | sed 's,\(.*\)\.o[ :]*,$(OBJDIR)/\1.o : ,g' > $(2)
endef
$(foreach cfile,$(CFILES),$(eval $(call rule4dfile, $(cfile), $(OBJDIR)/$(notdir $(cfile:.c=.d)))))

# rules to gen *.o file
define rule4ofile
$(2): $(1)
	$(CC) $(CFLAGS) -c $(1) -o $(2)
endef
$(foreach cfile,$(CFILES),$(eval $(call rule4ofile, $(cfile), $(OBJDIR)/$(notdir $(cfile:.c=.o)))))

clean:
	rm -f objs/*.d
	rm -f objs/*.o
	rm -f $(TARGET)
