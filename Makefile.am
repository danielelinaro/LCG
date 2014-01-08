ACLOCAL_AMFLAGS = -I m4
SUBDIRS = stimgen common entities streams engine bin
EXTRA_DIST = CHANGES LICENSE python
dist_doc_DATA = README.md
dist-hook:
	rm -rf $(distdir)/python/build
	rm -rf $(distdir)/python/dist
	rm -rf $(distdir)/python/lcg.egg-info
	rm -f $(distdir)/python/correlations_old.py
	rm -f `find $(distdir)/python -name '*.pyc'`
