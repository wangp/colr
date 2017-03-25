colr - colour lines based on regular expression
===============================================

Usage
-----

```
colr [-i] REGEXP [COLOUR...]
```

regexp is a POSIX extended regular expression.

colr reads lines from standard input and writes to standard output.

Each line is matched against the given regular expression.
Matches are performed case insensitively if the `-i` option is given.

Each submatch is coloured by introducing ANSI escape sequences to
the output. Colours can be assigned to each submatch in the arguments
following the regular expression. Colours are cycled if fewer are given
than exist submatches. The recognised colour names are: normal, red, green,
yellow, blue, magenta, cyan, or prefixed with "dark" for non-bold versions.

colr does not parse ANSI escape sequences in the input so strange things may
happen if the regular expression breaks up an existing escape sequence.

Example
-------

    ls -l | colr '^.(...)(...)(...)' normal cyan yellow red

Author
------
Peter Wang
