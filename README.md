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
following the regular expression. The list of colours is cycled if fewer
colours are given than the number of submatches.
Recognised colour names are:

  - normal
  - black, boldblack
  - darkred, red
  - darkgreen, green
  - darkyellow, yellow
  - darkblue, blue
  - darkmagenta, magenta
  - darkcyan, cyan
  - white, boldwhite

colr does not parse ANSI escape sequences in the input so strange things may
happen if the regular expression breaks up an existing escape sequence.

Example
-------

    ls -l | colr '^.(...)(...)(...)' normal cyan yellow red

Author
------
Peter Wang
