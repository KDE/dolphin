#ifndef _KONQSIDEBARIFACE_H_
#define _KONQSIDEBARIFACE_H_
class KonqSidebarIface {
public:
	KonqSidebarIface(){}
	virtual ~KonqSidebarIface(){}
	virtual bool universalMode()=0;
};
#endif
