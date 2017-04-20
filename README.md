LIBPUMPKIN
==========

ABOUT
-----

Libpumpkin is a free software collection of tools on top of Qt/C++,
from the server side (logging framework, small REST webapps, etc.)
to the desktop (widgets) and embedded (QML).

It's splitted into several build subprojects and binary libraries to avoid
too many dependencies. Subprojects are named using the "p6" prefix (short for
pumpkin: p followed by 6 letters) and a suffix that follow more or less Qt 5
libraries names: libp6core, libp6sql, etc.

See http://libpumpkin.g76r.eu/ for more information.

LICENSE
-------

This software is made availlable to you under the terms of the GNU Affero
General Public License version 3, see AGPL-3.0.txt file content.

BUILD INSTRUCTIONS
------------------

This software can be build with Qt's qmake build tool or a Qt-aware IDE
like Qt Creator.
