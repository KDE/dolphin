INCLUDES = -I$(top_srcdir)/libkonq $(all_includes) 
LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

lib_LTLIBRARIES = libkcm_konqhtml.la

libkcm_konqhtml_la_SOURCES = main.cpp htmlopts.cpp \
			khttpoptdlg.cpp miscopts.cpp

libkcm_konqhtml_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
libkcm_konqhtml_la_LIBADD = -lkdeui  

messages:
	$(XGETTEXT) $(libkcm_konqhtml_la_SOURCES) -o $(podir)/kcmkonqhtml.pot


#bin_PROGRAMS = test
#test_SOURCES = test.cpp rootopts.cpp main.cpp behaviour.cpp htmlopts.cpp \
#                        khttpoptdlg.cpp miscopts.cpp
#test.cpp:
#	echo "int main() {};" > test.cpp
#test_LDFLAGS  = $(all_libraries) -module -avoid-version
#test_LDADD = -lkdeui                                                    
