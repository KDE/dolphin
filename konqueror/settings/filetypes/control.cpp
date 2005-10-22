#include "filetypesview.h"
#include <kinstance.h>

extern "C"
{
	  KDE_EXPORT KCModule *create_filetypes(QWidget *parent, const char *)
          {
         KInstance *inst = new KInstance("filetypes");
        return new FileTypesView(inst, parent);
	  }

}

