
doc/index.html:
	doxygen doxygen.conf

doc: doc/index.html

clear:
	rm -rf doc/*

