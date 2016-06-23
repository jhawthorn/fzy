## 0.6 (Unreleased)

Performance:

  - Use threads to parallelize searching and scoring
  - Read all pending input from tty before searching

Bugfixes:

  - Fixed command line parsing on ARM

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
