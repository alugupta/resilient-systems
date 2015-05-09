SVGFILE= $(shell ls *.svg)
PDFFILE= $(subst .svg,.pdf,$(SVGFILE))

#INKSCAPE = /Applications/Inkscape.app/Contents/Resources/bin/inkscape

# UDIT'S INKSCAPE
INKSCAPE = inkscape

all:$(PDFFILE)

%.pdf : %.svg
	$(INKSCAPE) -z -D --file=$< --export-pdf=$@ 

clean:
	rm $(FILE)
