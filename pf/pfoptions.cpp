#include "pfoptions.h"
#include <QRegExp>

QString PfOptions::normalizeSurface(QString surface) {
  if (surface.isNull())
    return QString();
  QRegExp dupcolons("::+"), headstails("(^:+|:+$)"), illegals("[^a-zA-Z0-9:]"),
      nulls("(null:|:null)");
  surface.replace(illegals, "").replace(dupcolons, ":").replace(nulls, "")
      .replace(headstails, "");
  if (surface.isEmpty() || surface == "null")
    surface = "";
  return surface;
}
