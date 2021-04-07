# Description
"text-filter" is a module necessary for parsing server application logs, for debugging purposes.
The module is made using flex and bison generators. The source library uses pre-generated parsers and if you want to somehow fix or supplement this module, you need to install the appropriate packages.

## how to use:
use these commands to generate files:
```
bison -d -o grammar-filter.c grammar-filter.y
flex -o lexer-filter.c lexer-filter.l
```
`text-filter-example.c` contains an example of use. If you want to run it, go to the module directory and run the following commands:
```
autoreconf --force --install
./configure
make
./text-filter-example
```