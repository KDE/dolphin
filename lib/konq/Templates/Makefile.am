
# The links (that contain translations)
templ_DATA = linkProgram.desktop \
	linkURL.desktop linkFloppy.desktop linkHD.desktop linkCDROM.desktop \
	Directory.desktop TextFile.desktop HTMLFile.desktop

templdir = $(kde_templatesdir)

# The source files (the actual templates)
sources_DATA = Program.desktop \
        URL.desktop Floppy.desktop HD.desktop CDROM-Device.desktop \
	TextFile.txt HTMLFile.html

sourcesdir = $(kde_templatesdir)/.source

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(sourcesdir)/emptydir
