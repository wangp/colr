colr - colour lines based on regular expression
===============================================

Usage
-----

```
colr [-i] REGEXP
```

regexp is a POSIX extended regular expression.

colr reads lines from standard input and writes to standard output.

Each line is matched against the given regular expression.
Matches are performed case insensitively if the `-i` option is given.

Each submatch is coloured by introducing ANSI escape sequences to the output.
Currently, the colours cannot be selected.

colr does not parse ANSI escape sequences in the input so strange things may
happen if the regular expression breaks up an existing escape sequence.

Author
------
Peter Wang
