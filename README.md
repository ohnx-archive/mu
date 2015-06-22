# mu
not that this means much, but: [![Build Status](https://travis-ci.org/ohnx/mu.svg?branch=master)](https://travis-ci.org/ohnx/mu)
mu is a mini markdown server in C.

It has a configurable cache for parsed files, custom headers/footers for a file, and a bit more.

##about
mu parses Markdown using the hoedown library and the server is based on nweb by Nigel Griffiths.

mu can be used for small blogs and things like that, but it is not recommended that you use it with large traffic applications. This is primairly because it forks for each request, and does not support keep-alive. In the future, I might try making it event-based using libevent.

##installation
You will need gcc and access to the internet (will clone and build hoedown). Run ```./configure.sh``` then ```./install.sh```. Configure other options as needed in simple.cfg.

##other
feel free to poke around the code. it's semi-commented. honestly, there's nto too much, and the files are all split up.

##license
apache

##todo
 - add customizeable page template (no more header and footer) where with formatting strings (ie, %fname%, %path%, %content)
 - event based (probably will never get to this)
 - add ability to edit files? :o

made by ohnx :smile:
