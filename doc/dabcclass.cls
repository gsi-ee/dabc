% documentclass for the DABC manuals
\def\fileversion{1}
\def\filedate{2009/01/16}
\def\docdate{2009/01/16}

\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{dabcclass}[\filedate]
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{report}}
\PassOptionsToClass{a4paper,twoside,openright,english,11pt}{report}
\ProcessOptions
\LoadClass[a4paper,twoside,openright,english,11pt]{report}

\usepackage[pdftex]{graphicx}
\usepackage{pifont}
\usepackage{array,tabularx}
\usepackage{verbatim}
\usepackage[usenames]{color} % colors
\usepackage{varioref}
\usepackage{subfigure}
\usepackage{amsmath,amssymb}
\usepackage{cite}
%\usepackage{citesort}
\usepackage{bibunits}
\usepackage{here}
\usepackage{xspace}
\usepackage{calc,ifthen}
\usepackage{makeidx}
\usepackage{rotating}
\usepackage{mathptm}
\usepackage{multirow}
\usepackage{eurosym}
\usepackage{eso-pic}
\usepackage{here}
\usepackage{afterpage}   % to be used with \clearpage: allows for text after figures: \afterpage{\clearpage}
\usepackage{exscale}     % richtige Skalierung mathem. Formeln
\usepackage{fancyhdr}    % for defining new style for headings
\usepackage{t1enc}
\usepackage{enumerate}
\usepackage{euscript}
\usepackage[latin1]{inputenc}
\usepackage[T1]{fontenc}
\usepackage{paralist}  % compact lists
%\usepackage[pdftex,colorlinks]{hyperref}
\usepackage[pdftex,bookmarks=true,colorlinks=true,
                      pdfstartview=FitV,
                      linkcolor=blue,
                      citecolor=blue,
                      urlcolor=blue]{hyperref}
\def\insertindex{\@input{\jobname.ind}}
% replace Latex fonts with Postscript fonts
\renewcommand{\rmdefault}{ptm}
\renewcommand{\ttdefault}{pcr} 

% number of sections shown in "Contents", 4 ist standard
\setcounter{tocdepth}{5}


% number of sections which get a number
\setcounter{secnumdepth}{4}
% allows 4 levels of sections, e.g. '5.3.2 The Blablabla'
% sequence: \section \subsection \subsubsection \paragraph


% adjust spacing between rows in tables
\renewcommand{\arraystretch}{1.05}

% text settings
% --------------
\setlength{\voffset}{-3.5cm} 
\setlength{\hoffset}{-3cm}
\setlength{\topmargin}{1cm} 
\setlength{\oddsidemargin}{3.3cm}
\setlength{\evensidemargin}{2.6cm} 
\setlength{\textwidth}{16cm}
\setlength{\textheight}{24cm} 
\setlength{\parindent}{0cm} 
\setlength{\parskip}{0.2cm}
\setlength{\headheight}{2cm}
\setlength{\headsep}{1.3cm}
\setlength{\footskip}{1.3cm}


% float settings
% ---------------
\setcounter{topnumber}{3} \setcounter{bottomnumber}{3}
\setcounter{totalnumber}{10}
\renewcommand{\textfraction}{0.}
\renewcommand{\topfraction}{1.}
\renewcommand{\bottomfraction}{1.}
\renewcommand{\floatpagefraction}{0.99}



% define headings
% ---------------
\pagestyle{fancy}
\renewcommand{\chaptermark}[1]{\markboth{#1}{}}
\renewcommand{\sectionmark}[1]{\markright{\thesection.\ #1}{}}
\lhead[\fancyplain{}{\thepage}]%
      {\fancyplain{}{\rightmark}}
\rhead[\fancyplain{}{\leftmark}]%
      {\fancyplain{}{\thepage}}
\cfoot{}


% reduce textsize for captions
% -----------------------------
% typeset caption contents in small
\def\@makecaption#1#2{%
   \vskip 5\p@
   \setbox\@tempboxa\hbox{{\sl\bf #1:} {\small\rm #2}}%
   \ifdim \wd\@tempboxa >\hsize
       {\sl\bf #1:} {\small\rm #2}\par
     \else
       \hbox to\hsize{\hfil\box\@tempboxa\hfil}%
   \fi}


% paragraph bold with linebreak afterwards
% ----------------------------------------
\renewcommand{\paragraph}[1]{\vspace{0.7cm}\textbf{#1}\newline\vspace{-0.3cm}}

% define layout of chapter heading
% --------------------------------
\def\@makechapterhead#1{%
 %  \twocolumn[%
 \noindent\begin{minipage}{\textwidth}%
                 \raggedright
 \huge\bf  \thechapter\quad #1\par
            \vspace*{-3mm}%
            \rule{\textwidth}{0.8pt}%
            \vspace*{+8mm}%
       \end{minipage}%
  % ]%
}

\def\@makeschapterhead#1{%
 %  \twocolumn[%
\noindent\begin{minipage}{\textwidth}%
                 \raggedright
                \huge\bf #1\par
            \vspace*{-3mm}%
            \rule{\textwidth}{0.8pt}%
            \vspace*{+8mm}%
       \end{minipage}%
 %  ]%
}

\def\chapter{\thispagestyle{plain}\secdef\@chapter\@schapter}

\def\@chapter[#1]#2{\ifnum \c@secnumdepth >\m@ne
        \refstepcounter{chapter}%
        \typeout{\@chapapp\space\thechapter.}%
        \addcontentsline{toc}{chapter}{\protect
        \numberline{\thechapter}#1}\else
      \addcontentsline{toc}{chapter}{#1}\fi
   \chaptermark{#1}%
   \addtocontents{lof}%
       {\protect\addvspace{10\p@}}
   \addtocontents{lot}%
       {\protect\addvspace{10\p@}}
   \@makechapterhead{#1}
}

\def\Chapter#1{\addcontentsline{toc}{chapter}{#1}%
       \markboth{}{}\noindent
       \begin{minipage}{\textwidth}%
                 \raggedright
                \huge\bf #1\par
            \vspace*{-3mm}%
            \rule{\textwidth}{0.8pt}%
            \vspace*{+8mm}%
       \end{minipage}%
       \thispagestyle{plain}%
}

\def\@schapter#1{\@makeschapterhead{#1}}


% change "bibliography" (standard for report) to "references"
% and adopt layout of heading to those of chapters
% ------------------------------------------------------------
\def\bibname{References}
\def\thebibliography#1{\par\list
  {\@biblabel{\arabic{enumiv}}}{\settowidth\labelwidth{\@biblabel{#1}}%
    \leftmargin\labelwidth
    \advance\leftmargin\labelsep
    \usecounter{enumiv}%
    \let\p@enumiv\@empty
    \def\theenumiv{\arabic{enumiv}}}%
    \def\newblock{\hskip .11em plus.33em minus.07em}%
    \sloppy\clubpenalty4000\widowpenalty4000
    \sfcode`\.=1000\relax}
    
% somehow needed for correct chapter record in table of contents...    
% -----------------------------------------------------------------
\renewcommand*\l@chapter[2]{%
   %mg\addpenalty{-\@highpenalty}%
    \vskip 1.0em \@plus\p@
    \setlength\@tempdima{1.5em}%
    \begingroup
      \parindent \z@ \rightskip \@pnumwidth
      \parfillskip -\@pnumwidth
      \leavevmode \bfseries
      \advance\leftskip\@tempdima
      \hskip -\leftskip
      #1\nobreak\hfil \nobreak\hb@xt@\@pnumwidth{\hss #2}\par
      \penalty\@highpenalty
    \endgroup
}


% adjust indent for table of content to less than standard indent
% ----------------------------------------------------------------
\renewcommand{\l@section}{\@dottedtocline{1}{0.6em}{2.9em}}
\renewcommand{\l@subsection}{\@dottedtocline{2}{1.2em}{3.8em}}
\renewcommand{\l@subsubsection}{\@dottedtocline{3}{2.5em}{4.0em}}
\renewcommand{\l@paragraph}{\@dottedtocline{4}{3.5em}{4.5em}}
\renewcommand{\l@subparagraph}{\@dottedtocline{5}{4.5em}{5.0em}}

% semantic fontstyles
% -----------------------------------------------------------------
\newcommand{\verba}[1]{{\tt {\large #1}}}
\newcommand{\decl}[1]{{\tt #1}}
\newcommand{\class}[1]{{\bf{\em #1}}}
\newcommand{\func}[1]{{\sl #1}}
\newcommand{\member}[1]{{\sl #1}}
\newcommand{\strong}[1]{{\bf #1}}
\newcommand{\keyw}[1]{{\sf #1}}
\newcommand{\param}[1]{{\sf #1}}
\newcommand{\comm}[1]{{\sf #1}}

% compact lists etc
% -----------------------------------------------------------------
\newcommand{\bbul}{\begin{compactitem}[$\bullet$]}
\newcommand{\ebul}{\end{compactitem}}
\newcommand{\bcir}{\begin{compactitem}[$\circ$]}
\newcommand{\ecir}{\end{compactitem}}
\newcommand{\btri}{\begin{compactitem}[$\triangleright$]}
\newcommand{\etri}{\end{compactitem}}
\newcommand{\bbox}{\begin{compactitem}[$\Box$]}
\newcommand{\ebox}{\end{compactitem}}
\newcommand{\bnum}{\begin{compactenum}}
\newcommand{\enum}{\end{compactenum}}
\newcommand{\bdes}{\begin{compactdesc}}
\newcommand{\edes}{\end{compactdesc}}

\endinput
