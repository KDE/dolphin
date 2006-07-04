


########### install files ###############

install( FILES linkProgram.desktop  	linkURL.desktop linkFloppy.desktop linkHD.desktop linkCDROM.desktop  	Directory.desktop TextFile.desktop HTMLFile.desktop linkZIP.desktop  	linkDVDROM.desktop linkCAMERA.desktop linkNFS.desktop linkCDWRITER.desktop  	linkMO.desktop  DESTINATION  ${TEMPLATES_INSTALL_DIR} )
install( FILES Program.desktop  	URL.desktop Floppy.desktop HD.desktop CDROM-Device.desktop  	TextFile.txt HTMLFile.html ZIP-Device.desktop DVDROM-Device.desktop  	CAMERA-Device.desktop NFS.desktop CDWRITER-Device.desktop  	MO-Device.desktop  DESTINATION  ${TEMPLATES_INSTALL_DIR}/.source )




#original Makefile.am contents follow:

#
## The links (that contain translations)
#templ_DATA = linkProgram.desktop \
#	linkURL.desktop linkFloppy.desktop linkHD.desktop linkCDROM.desktop \
#	Directory.desktop TextFile.desktop HTMLFile.desktop linkZIP.desktop \
#	linkDVDROM.desktop linkCAMERA.desktop linkNFS.desktop linkCDWRITER.desktop \
#	linkMO.desktop
#
#templdir = $(kde_templatesdir)
#
## The source files (the actual templates)
#sources_DATA = Program.desktop \
#	URL.desktop Floppy.desktop HD.desktop CDROM-Device.desktop \
#	TextFile.txt HTMLFile.html ZIP-Device.desktop DVDROM-Device.desktop \
#	CAMERA-Device.desktop NFS.desktop CDWRITER-Device.desktop \
#	MO-Device.desktop
#
#sourcesdir = $(kde_templatesdir)/.source
#
#install-data-hook:
#	$(mkinstalldirs) $(DESTDIR)$(sourcesdir)/emptydir
