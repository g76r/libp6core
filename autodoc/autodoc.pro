autodoc.commands = [ -z "$(AUTODOC)" ] || (doxygen && (cd latex; make))
QMAKE_EXTRA_TARGETS += autodoc
PRE_TARGETDEPS += $$autodoc.target
