all: 

README.html: ../progdesc.php docmaker.php Makefile
	echo "php -q docmaker.php ${ARCHNAME} $< > $@"
	@php -q docmaker.php "${ARCHNAME}" "$<" > .$@.tmp && mv -f .$@.tmp $@

%.html: source/%.php docmaker.php Makefile
	echo "php -q docmaker.php ${ARCHNAME} $< > $@"
	@php -q docmaker.php "${ARCHNAME}" "$<" > .$@.tmp && mv -f .$@.tmp $@
