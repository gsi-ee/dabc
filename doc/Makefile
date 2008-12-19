all: documents

documents: \
	main-all.pdf
main-all.pdf : main-all.tex \
user/steer-user.tex \
user/user-docrev.tex \
user/user-section.tex \
reference/steer-ref.tex \
reference/ref-docrev.tex \
reference/ref-section.tex \
./test/test-udapl.tex \
./test/test-fzk.tex \
./test/test-intro.tex \
./test/test-xdaq.tex \
./test/steer-test.tex \
./test/test-docrev.tex \
./introduction/in-docrev.tex \
./introduction/steer-intro.tex \
./introduction/in-intro.tex \
./simulation/simu-simu.tex \
./simulation/simu-docrev.tex \
./simulation/steer-simu.tex \
./dabc/dabc-design.tex \
./dabc/dabc-concept.tex \
./dabc/dabc-docrev.tex \
./dabc/steer-dabc.tex \
./software/sw-intro.tex \
./software/sw-docrev.tex \
./software/steer-software.tex \
./hardware/hw-intro.tex \
./hardware/steer-hardware.tex \
./hardware/hw-docrev.tex \
./templates/XXX-docrev.tex \
./templates/steer-XXX.tex \
./templates/XXX-section.tex \
./controls/steer-controls.tex \
./controls/ctrl-docrev.tex \
./controls/ctrl-intro.tex \
./environment/dabc-defs.tex \
./environment/dabc-docrev.tex \
./environment/dabc-frontpage.tex \
./environment/dabc-people.tex \
./environment/dabc-post.tex \
./environment/dabc-preface.tex \
./environment/dabc-work.tex \
./dtor/dtor-concept.tex \
./dtor/dtor-docrev.tex \
./dtor/steer-dtor.tex \
./dtor/dtor-design.tex \
./dabc-bibitems.tex \
./dabc-glossary.tex \
./dabc-requirements.tex
	pdflatex main-all.tex
	makeindex main-all.idx
	pdflatex main-all.tex
clean:
	rm -f main-all.pdf *.aux *.idx *.ilg *.ind *.log *.out *.toc
	