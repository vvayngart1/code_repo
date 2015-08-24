# c:    $(CC) -c $(CPPFLAGS) $(CFLAGS)
# c++:  $(CXX) -c $(CPPFLAGS) $(CXXFLAGS)
# app:  $(CC) $(LDFLAGS) n.o $(LOADLIBES) $(LDLIBS)

ifeq ($(CFG),)
CFG=debug
endif

ifeq ($(ONIX_VER),)
ONIX_VER=legacy
endif

include Makefile.incl/Makefile.env

srcroot = src/cpp
xml_srcroot = src/xml
sql_srcroot = src/sql
lib_srcroot = $(srcroot)/lib
mains_srcroot = $(srcroot)/mains

LDFLAGS := $(patsubst %,-L%,$(TW_LIBDIR)) $(TW_LDFLAGS) -pthread -mtune=native -march=native
CPPFLAGS := $(patsubst %,-I%,$(TW_INCL)) -I$(srcroot) -I$(lib_srcroot) $(TW_CPPFLAGS)
LDLIBS := $(patsubst %,-l%,$(TW_LIB))

ifeq ($(CFG),debug)
CFLAGS = -pthread -g -O0 -mtune=native -march=native -Wall -Werror -Wno-unused-function -ferror-limit=10 -fno-strict-aliasing -pipe $(TW_CFLAGS)
CXXFLAGS = -pthread -g -O0 -mtune=native -march=native -Wall -Werror -Wno-unused-function -ferror-limit=10 -fno-strict-aliasing -pipe $(TW_CXXFLAGS)
else ifeq ($(CFG),release)
#CFLAGS = -pthread -mtune=native -march=native -pg -O2 -Wall -Wno-unused-function -fno-strict-aliasing -pipe $(TW_CFLAGS)
#CXXFLAGS = -pthread -mtune=native -march=native -pg -O2 -Wall -Wno-unused-function -fno-strict-aliasing -pipe $(TW_CFLAGS)
CFLAGS = -pthread -g -O3 -mtune=native -march=native -Wall -Werror -Wno-unused-function -ferror-limit=10 -fno-strict-aliasing -pipe $(TW_CFLAGS)
CXXFLAGS = -pthread -g -O3 -mtune=native -march=native -Wall -Werror -Wno-unused-function -ferror-limit=10 -fno-strict-aliasing -pipe $(TW_CXXFLAGS)
else 
$(error no known configuration)
endif

ifeq ($(ONIX_VER),legacy)
CXXFLAGS += -DONIX_VER_LEGACY
else
CXXFLAGS += -DONIX_VER_UPDATE
endif

CXXFLAGS += -D__STDC_CONSTANT_MACROS -MMD

out_gendir = $(lib_srcroot)/tw/generated
out_sql_gendir = $(sql_srcroot)/generated
# TODO: need to figure out dependencies checking
# when 
#out_depdir = $(CFG)/dep
out_objdir = $(CFG)/obj
out_bindir = $(CFG)/bin
out_libdir = $(CFG)/lib

gen_h_sources =  $(shell find $(xml_srcroot) -type f -name '*.h.xsl')
gen_sql_sources =  $(shell find $(xml_srcroot) -type f -name '*.sql.xsl')
link_sources =  $(shell find $(lib_srcroot) -type f -name '*.c')
link_sources += $(shell find $(lib_srcroot) -type f -name '*.cpp')
main_sources = $(shell find $(mains_srcroot) -type f -name '*.c')
main_sources += $(shell find $(mains_srcroot) -type f -name '*.cpp')
main_standalones = $(shell find $(mains_srcroot) -maxdepth 1 -type f -name '*.c')
main_standalones += $(shell find $(mains_srcroot) -maxdepth 1 -type f -name '*.cpp')
main_directories = $(filter-out $(mains_srcroot),$(shell find $(mains_srcroot) -maxdepth 1 -type d))

gen_h_objects := $(foreach f,$(notdir $(basename $(gen_h_sources))),$(out_gendir)/$f)
gen_sql_objects := $(foreach f,$(notdir $(basename $(gen_sql_sources))),$(out_sql_gendir)/$f)
link_objects := $(addsuffix .o,$(patsubst $(srcroot)/%,$(out_objdir)/%,$(basename $(link_sources))))
main_objects := $(addsuffix .o,$(patsubst $(srcroot)/%,$(out_objdir)/%,$(basename $(main_sources))))
objects      := $(link_objects) $(main_objects)

main_standalone_mains := $(patsubst $(mains_srcroot)/%,$(out_bindir)/%,$(basename $(main_standalones)))
main_directory_mains := $(patsubst $(mains_srcroot)/%,$(out_bindir)/%,$(main_directories))
	
unit_tests_standalones = $(shell find $(mains_srcroot) -maxdepth 1 -type d -name 'unit_test_*')
unit_tests_standalone_mains := $(patsubst $(mains_srcroot)/%,$(out_bindir)/%,$(basename $(unit_tests_standalones)))

.PHONY : all clean doc verbose
.DELETE_ON_ERROR : $(mains)


all : $(gen_sql_objects) $(out_libdir)/libtw.a $(main_standalone_mains) $(main_directory_mains)

# pull in dependency info for *existing* .o files
-include $(objects:.o=.d)

$(out_gendir)/version.h :
	@echo Creating version.h from git
	./bin/version.sh $(out_gendir)

obj : $(link_objects) $(main_objects)
	
clean :
	-rm -rf ./doc/ ./debug/ ./release/ ./profile/ ./$(out_gendir)/ ./$(out_sql_gendir)/

doc :
	$(DOXYGEN) etc/doxygen.conf
	
#generate : $(gen_h_sources) $(gen_h_objects)

unit_tests :
	#@echo "Running unit tests"
	#@echo "..."
	#@echo
	for i in $(unit_tests_standalone_mains) ; do \
	    echo "Running: ==> " $$i " <=="; \
	    echo "..."; \
	    ./$$i; \
	    echo ""; \
	    echo "Done Running: ==> " $$i " <=="; \
	    echo ""; \
	done

generate_cpp : $(gen_h_objects)

generate_sql : $(gen_sql_objects)

verbose :
	@echo "Printing Makefile's variables"
	@echo "..."
	@echo
	@echo linkpath: $(CPPFLAGS) $(LDFLAGS) $(LDLIBS)	
	@echo
	@echo xml_srcroot: $(xml_srcroot)
	@echo out_gendir: $(out_gendir)
	@echo gen_h_sources: $(gen_h_sources)
	@echo gen_h_objects: $(gen_h_objects)
	@echo	
	@echo link_headers: $(link_headers)
	@echo link_sources: $(link_sources)
	@echo main_sources: $(main_sources)
	@echo main_standalones: $(main_standalones)
	@echo main_directories: $(main_directories)
	@echo
	@echo link_objects: $(link_objects)
	@echo main_objects: $(main_objects)
	@echo objects: $(objects)
	@echo
	@echo main_standalone_mains: $(main_standalone_mains)
	@echo main_directory_mains: $(main_directory_mains)
	@echo
	@echo unit_tests_standalones: $(unit_tests_standalones)
	@echo unit_tests_standalone_mains: $(unit_tests_standalone_mains)
	@echo
	@echo "Done"

$(out_libdir)/libtw.a : $(out_gendir)/version.h $(gen_h_objects) $(link_objects) 
	-@mkdir -p $(out_libdir)
	@echo "====="
	@echo "      ARing lib:" $@ ":: command line:" src/perl/ar.pl $(out_objdir)/lib $(AR) crus $@ $(link_objects)
	@echo "      ..."	
	@src/perl/ar.pl $(out_objdir)/lib $(AR) crus $@ $(link_objects)
	@touch $(out_libdir)/libtw.a
	@echo "      DONE -- ARing lib:" $@

$(main_standalone_mains) : $(out_bindir)/% : $(out_objdir)/mains/%.o $(out_libdir)/libtw.a
	-@mkdir -p $(out_bindir)
	@echo "====="
	@echo "      Linking" $@ ":: command line:" $(CXX) -o $@ $(LDFLAGS) $< $(TW_RPATH) -L$(out_libdir) -ltw $(LDLIBS)
	@echo "      ..."
	@$(CXX) -o $@ $(LDFLAGS) $< $(TW_RPATH) -L$(out_libdir) -ltw $(LDLIBS)
	@echo "      DONE -- Linking:" $@

# TODO every directory-main depends on all main object code.
# must reduce this dependency to only object code underneath each subdirectory
$(main_directory_mains) : $(out_bindir)/% : $(main_objects) $(out_libdir)/libtw.a
	-@mkdir -p $(out_bindir)
	@echo "====="
	@echo "      Linking" $@ ":: command line:" $(CXX) -pthread -o $@ $(LDFLAGS) $(filter $(out_objdir)/mains/$(notdir $@)/%,$^) $(TW_RPATH) -L$(out_libdir) -ltw $(LDLIBS)
	@echo "      ..."
	@$(CXX) -pthread -o $@ $(LDFLAGS) $(filter $(out_objdir)/mains/$(notdir $@)/%,$^) $(TW_RPATH) -L$(out_libdir) -ltw $(LDLIBS)
	@echo "      DONE -- Linking:" $@

$(out_gendir)/%.h : $(xml_srcroot)/%.h.xsl $(xml_srcroot)/%.xml
	@mkdir -p $(out_gendir)
	@echo "====="
	@echo "      XSLT Generating:" $@ ":: command line:" $(XSLTPROC) -o $@ $(xml_srcroot)/$(notdir $@).xsl $(xml_srcroot)/$(basename $(notdir $@)).xml
	@echo "      ..."
	$(XSLTPROC) -o $@ $(xml_srcroot)/$(notdir $@).xsl $(xml_srcroot)/$(basename $(notdir $@)).xml
	@echo "      DONE -- XSLT Generating:" $@

$(out_sql_gendir)/%.sql : $(xml_srcroot)/%.sql.xsl $(xml_srcroot)/%.xml
	@mkdir -p $(out_sql_gendir)
	@echo "====="
	@echo "      XSLT Generating:" $@ ":: command line:" $(XSLTPROC) -o $@ $(xml_srcroot)/$(notdir $@).xsl $(xml_srcroot)/$(basename $(notdir $@)).xml
	@echo "      ..."
	$(XSLTPROC) -o $@ $(xml_srcroot)/$(notdir $@).xsl $(xml_srcroot)/$(basename $(notdir $@)).xml
	@echo "      DONE -- XSLT Generating:" $@

$(out_objdir)/%.o : $(srcroot)/%.c
	@mkdir -p $(dir $@)
	@echo "====="
	@echo "      Compiling:" $< ":: command line:" $(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@
	@echo "      ..."
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@
	@echo "      DONE -- Compiling:" $<

$(out_objdir)/%.o : $(srcroot)/%.cpp
	@mkdir -p $(dir $@)
	@echo "====="	
	@echo "      Compiling:" $< ":: command line:" $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
	@echo "      ..."
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
	@echo "      DONE -- Compiling:" $<
