:: one step:
::	xelatex halo.tex
latex halo.tex
dvipdfmx halo.dvi
del *.log
del *.aux
del *.dvi
mv halo.pdf ../../../../_bin/