#include "filetypesview.h"

extern "C"
{
	  KDE_EXPORT KCModule *create_filetypes(QWidget *parent, const char *)
          {
        return new FileTypesView(parent, "filetypes");
	  }

}

