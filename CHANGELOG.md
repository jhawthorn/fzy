## 1.0 (2018-09-23)

Features:

  - Support UTF-8
  - Support readline-like editing
  - Quit on Esc
  - Redraw on terminal resize
  - Bracketed paste escapes are ignored

Performance:

  - Initialize tty interface before reading stdin

## 0.9 (2017-04-17)

Features:

  - Support Ctrl-k and Ctrl-j for movement

Performance:

  - Use threads to parallelize sorting
  - Improve parallelism of searching and scoring

Internal:

  - Fix test suite on i386
  - Replace test suite with greatest
  - Add property tests
  - Add acceptance tests

## 0.8 (2017-01-01)

Bugfixes:

  - Fix cursor position shifing upwards when input has less than 2 items.

## 0.7 (2016-08-03)

Bugfixes:

  - Fixed a segfault when encountering non-ascii characters
  - Fixed building against musl libc

## 0.6 (2016-07-26)

Performance:

  - Use threads to parallelize searching and scoring
  - Read all pending input from tty before searching
  - Use a lookup table for computing bonuses

Bugfixes:

  - Fixed command line parsing on ARM
  - Fix error when autocompleting and there are no matches

## 0.5 (2016-06-11)

Bugfixes:

  - Made sorting stable on all platforms

## 0.4 (May 19, 2016)

Features:

  - Add `-q`/`--query` for specifying initial query

Bugfixes:

  - Fixed last line of results not being cleared on exit
  - Check errors when opening the TTY device

## 0.3 (April 25, 2016)

Bugfixes:

  - Runs properly in a terminal with -icrnl

## 0.2 (October 19, 2014)

Features:

  - Allow specifying custom prompt

Performance:

  - Reduce memory usage on large sets

Bugfixes:

  - Terminal is properly reset on exit
  - Fixed make install on OS X

## 0.1 (September 20, 2014)

Initial release
