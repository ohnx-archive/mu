#!/bin/bash
git clone https://github.com/hoedown/hoedown.git
cd hoedown
make
cd src
cp * ../
cd ../../
cc -O3 -Ihoedown -fPIC -c -o dynpages.o dynpages.c
cc -O3 -c -o cfghelper.o cfghelper.c
cc -O3 -c -Iinclude -fPIC -o server.o server.c
cc server.o dynpages.o cfghelper.o hoedown/autolink.o hoedown/buffer.o hoedown/document.o hoedown/escape.o hoedown/html.o hoedown/html_blocks.o hoedown/html_smartypants.o hoedown/stack.o hoedown/version.o -o mu
