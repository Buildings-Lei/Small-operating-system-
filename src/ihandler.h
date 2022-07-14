
#ifndef IHANDLER_H
#define IHANDLER_H

#define DeclHandler(name)    void name##Entry(); \
                             void name()

DeclHandler(SegmentFaultHandler);   
DeclHandler(PageFaultHandler);                             
DeclHandler(TimerHandler);
DeclHandler(KeyboardHandler);
DeclHandler(SysCallHandler);

#endif
