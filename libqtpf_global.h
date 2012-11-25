#ifndef LIBQTPF_GLOBAL_H
#define LIBQTPF_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LIBQTPF_LIBRARY)
#  define LIBQTPFSHARED_EXPORT Q_DECL_EXPORT
#else
#  define LIBQTPFSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // LIBQTPF_GLOBAL_H
