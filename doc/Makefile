all: documents

documents: \
	main-all.pdf
main-all.pdf : main-all.tex \
dabcclass.cls \
templates/XXX-docrev.tex \
templates/steer-XXX.tex \
templates/XXX-section.tex \
environment/dabc-defs.tex \
environment/dabc-docrev.tex \
environment/dabc-frontpage.tex \
environment/dabc-people.tex \
environment/dabc-post.tex \
environment/dabc-editorial.tex \
environment/dabc-preface.tex \
environment/dabc-work.tex \
user/steer-user.tex \
user/user-docrev.tex \
user/user-command.tex \
user/user-parameter.tex \
user/user-plugin.tex \
user/user-setup.tex \
user/user-overview.tex \
user/user-exa-mbs.tex \
user/user-exa-bnet.tex \
user/user-exa-roc.tex \
programmer/steer-programmer.tex \
programmer/prog-docrev.tex \
programmer/prog-concepts.tex \
programmer/prog-command.tex \
programmer/prog-parameter.tex \
programmer/prog-plugin.tex \
programmer/prog-setup.tex \
programmer/prog-overview.tex \
programmer/prog-exa-mbs.tex \
programmer/prog-exa-bnet.tex \
programmer/prog-exa-roc.tex \
programmer/prog-gui.tex \
reference/steer-reference.tex \
reference/ref-docrev.tex \
reference/ref-overview.tex \
controls/steer-controls.tex \
controls/ctrl-docrev.tex \
controls/ctrl-intro.tex \
controls/ctrl-command.tex \
controls/ctrl-parameter.tex \
controls/ctrl-Dim.tex \
controls/ctrl-DimJavaGui.tex \
controls/ctrl-setup.tex \
controls/ctrl-statem.tex \
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
	pdflatex main-all.tex
	rm -f *.aux *.idx *.ilg *.ind *.log *.out *.toc
clean:
	rm -f main-all.pdf *.aux *.idx *.ilg *.ind *.log *.out *.toc
	