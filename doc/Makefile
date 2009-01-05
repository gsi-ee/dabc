all: documents

documents: \
	main-all.pdf
main-all.pdf : main-all.tex \
templates/XXX-docrev.tex \
templates/steer-XXX.tex \
templates/XXX-section.tex \
environment/dabc-defs.tex \
environment/dabc-docrev.tex \
environment/dabc-frontpage.tex \
environment/dabc-people.tex \
environment/dabc-post.tex \
environment/dabc-preface.tex \
environment/dabc-work.tex \
user/steer-user.tex \
user/user-docrev.tex \
user/user-section.tex \
reference/steer-reference.tex \
reference/ref-docrev.tex \
reference/ref-section.tex \
controls/steer-controls.tex \
controls/ctrl-docrev.tex \
controls/ctrl-intro.tex \
introduction/in-docrev.tex \
introduction/steer-introduction.tex \
introduction/in-intro.tex \
introduction/in-dabc-concept.tex \
introduction/in-dabc-design.tex \
test/test-udapl.tex \
test/test-fzk.tex \
test/test-intro.tex \
test/test-xdaq.tex \
test/steer-test.tex \
test/test-docrev.tex \
simulation/simu-simu.tex \
simulation/simu-docrev.tex \
simulation/steer-simulation.tex \
software/sw-intro.tex \
software/sw-docrev.tex \
software/steer-software.tex \
hardware/hw-intro.tex \
hardware/steer-hardware.tex \
hardware/hw-docrev.tex \
dtor/dtor-concept.tex \
dtor/dtor-docrev.tex \
dtor/steer-dtor.tex \
dtor/dtor-design.tex \
dabc-bibitems.tex \
dabc-glossary.tex \
dabc-requirements.tex
	pdflatex main-all.tex
	makeindex main-all.idx
	pdflatex main-all.tex
	rm -f *.aux *.idx *.ilg *.ind *.log *.out *.toc
clean:
	rm -f main-all.pdf *.aux *.idx *.ilg *.ind *.log *.out *.toc
	