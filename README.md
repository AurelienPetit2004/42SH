With Autotools, this 42sh will be compiled using the following commands and should appear under src/42sh:

42sh$ autoreconf --install

...

42sh$ ./configure

...

42sh$ make

...

42sh$ ./src/42sh

The binary should be created under src/42sh. When testing, this 42sh will be installed at a given path using the following commands:

42sh$ autoreconf --install

...

42sh$ ./configure --prefix=/install/at/this/path

...

42sh$ make install

...

42sh$ /install/at/this/path/bin/42sh

You can then retrieve the 42sh binary at the chosen path (/install/at/this/path/bin/42sh).
