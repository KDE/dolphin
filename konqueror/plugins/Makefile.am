
SUBDIRS = searcher

messages:
	for subdir in $(SUBDIRS); do \
           if test ! "x$$subdir" = "x."; then \
           echo "Making messages in $$subdir"; \
           (cd $$subdir && $(MAKE) -f Makefile.am messages) || exit 1; \
         fi ;\
        done 
