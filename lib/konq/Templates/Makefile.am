
# The links (that contain translations)
templ_DATA = linkProgram.desktop \
	linkURL.desktop linkFloppy.desktop linkCDROM.desktop \
	Directory.desktop TextFile.desktop HTMLFile.desktop

templdir = $(kde_templatesdir)

# The source files (the actual _templates)
sources_DATA = Program.desktop \
        URL.desktop Floppy.desktop CDROM-Device.desktop \
	TextFile.txt HTMLFile.html

sourcesdir = $(kde_templatesdir)/.source

install-data-local:
	@if test ! -d $(DESTDIR)/$(sourcesdir)/emptydir; then \
	  mkdir $(DESTDIR)/$(sourcesdir)/emptydir; \
	fi
	-rm -f $(DESTDIR)/$(kde_templatesdir)/linkFtpURL.desktop
	-rm -f $(DESTDIR)/$(kde_templatesdir)/linkWWWURL.desktop

