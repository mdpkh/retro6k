# Retro 6k Fantasy Computer Entertainment System

## Where are the docs?

Because the finished PDF of the Programmer's Guide (and presumably of other documents once they contain significant content) is rather large and opaque to Git's differencing system, fully-assembled PDFs of the guides will not be included in this source repository. See the following sections for ways to obtain the guides in PDF format.

## Build from source

Use `pdflatex` to compile [pg-src/r6k-pg.tex](pg-src/r6k-pg.tex). On the first run, the Table of Contents will be empty, cross-references will be broken, and some tables may have weird layouts. Run `pdflatex` a couple more times to sort that out. Then, so it's easier to find, move the resulting `pg-src/r6k-pg.pdf` to `.`. Do the same for [eug-src/r6k-eug.tex](eug-src/r6k-eug.tex) and [baug-src/r6k-baug.tex](baug-src/r6k-baug.tex). In the future, all this can be achieved by typing `make docs` in the `retro6k` root directory, but we haven't taken care of setting that up yet.

A bare installation of LaTeX may be missing several packages and a font or two. The MikTeX suite on Windows should take care of installing those automatically; otherwise, consult your LaTeX distribution documentation.

## Releases

Retro 6k releases are planned to include fully-assembled documentation PDFs.

## Author's personal web space

To supplement the above mechanisms, I provide copies of the documentation on my own web space. They are not guaranteed to be up-to-date.
* [Bundled Application User's Guide](http://vidthekid.info/retro6k/r6k-baug.pdf)
* [Emulator User's Guide](http://vidthekid.info/retro6k/r6k-eug.pdf)
* [Programmer's Guide](http://vidthekid.info/retro6k/r6k-pg.pdf)

---

*pretend this part is right-aligned — MDPKH*
