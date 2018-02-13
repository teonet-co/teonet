# File : Makefile.PL
use ExtUtils::MakeMaker;
WriteMakefile(
    'NAME'    => 'teonet',                  # Name of package
    'LIBS'    => ['-lteonet'],              # Name of custom libraries
    'OBJECT'  => 'teonet_wrap.o',   	    # Object files
    'AUTHOR'  => ['Kirill Scherba <kirill@scherba.ru>'],
);
