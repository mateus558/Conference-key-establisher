#ifndef IOTKEYAGREEMENTCORE_GLOBAL_H
#define IOTKEYAGREEMENTCORE_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(IOTKEYAGREEMENTCORE_LIBRARY)
#  define IOTKEYAGREEMENTCORE_EXPORT Q_DECL_EXPORT
#else
#  define IOTKEYAGREEMENTCORE_EXPORT Q_DECL_IMPORT
#endif

#endif // IOTKEYAGREEMENTCORE_GLOBAL_H