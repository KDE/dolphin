
# The links (that contain translations)
templ_DATA = linkFtpURL.desktop linkProgram.desktop linkWWWURL.desktop \
	linkURL.desktop linkFloppy.desktop linkCDROM.desktop \
	Directory.desktop TextFile.desktop HTMLFile.desktop

templdir = $(kde_templatesdir)

# The source files (the actual _templates)
sources_DATA = FtpURL.desktop Program.desktop WWWURL.desktop \
        URL.desktop Floppy.desktop CDROM-Device.desktop \
	TextFile.txt HTMLFile.html

sourcesdir = $(kde_templatesdir)/.source

install-data-local:
	@if test ! -d $(DESTDIR)/$(sourcesdir)/emptydir; then \
	  mkdir $(DESTDIR)/$(sourcesdir)/emptydir; \
	fi

