all: 

README.html: ../progdesc.php docmaker.php Makefile
	php -q docmaker.php "${ARCHNAME}" "$<" > .$@.tmp && mv -f .$@.tmp $@

%.html: %.php docmaker.php Makefile
	php -q docmaker.php "${ARCHNAME}" "$<" > .$@.tmp && mv -f .$@.tmp $@
