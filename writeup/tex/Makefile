OBJ = obj
LATEX = pdflatex -output-directory $(OBJ)
BIBTEX = bibtex
DVIPS = dvips
PS2PDF = ps2pdf

MAIN_DOC = main

all:
	mkdir -p $(OBJ);
	$(LATEX) $(MAIN_DOC).tex;
	$(BIBTEX) $(OBJ)/$(MAIN_DOC);
	$(LATEX) $(MAIN_DOC).tex;
	$(LATEX) $(MAIN_DOC).tex;
	cp $(OBJ)/$(MAIN_DOC).pdf .

fig:
	cd figs && make

tar:
	rm -f $(MAIN_DOC).tar.gz $(MAIN_DOC).tar; tar -cf $(MAIN_DOC).tar *.sty *.tex *.bib makefile $(MAIN_DOC).ps.gz EPS/*.eps TAB/*.tex outline; gzip $(MAIN_DOC).tar;


clean:
	rm -rf   \
                *.aux \
                *.lot \
                *.lof \
                *.bbl \
                *.toc \
                *.blg \
                *.dvi \
                *.log \
                *.out \
                *.ps  \
                *~ \
                BIB/*~ \
                $(OBJ)

