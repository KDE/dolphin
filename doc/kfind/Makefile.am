
data_DATA =  index.html index-1.html index-2.html \
         index-3.html index-4.html logotp3.gif 

datadir = $(kde_htmldir)/en/kfind

EXTRA_DIST = $(data_DATA) index.sgml index.nif

doc: index.sgml index.nif
	ksgml2html index en

