
This document describes the scoring algorithm of fzy as well as the algorithm
of other similar projects.

# Matching vs Scoring

I like to split the problem a fuzzy matchers into two subproblems: matching and scoring.

Matching determines which results are eligible for the list.
All the projects here consider this to be the same problem, matching the
candidate strings against the search string with any number of gaps.

Scoring determines the order in which the results are sorted.
Since scoring is tasked with finding what the human user intended, there is no
correct solution. As a result there are large variety in scoring strategies.

# fzy's matching

Generally, more time is taken in matching rather than scoring, so it is
important that matching be as fast as possible. If this were case sensitive it
would be a simple loop calling strchr, but since it needs to be case
insensitive.

# fzy's scoring

fzy treats scoring as a modified [edit
distance](https://en.wikipedia.org/wiki/Edit_distance) problem of calculating
the
[Levenshtein distance](https://en.wikipedia.org/wiki/Levenshtein_distance).
Edit distance is the measure of how different two strings are in terms of
insertions, deletions, and substitutions. This is the same problems as [DNA
sequence alignment](https://en.wikipedia.org/wiki/Sequence_alignment). Fuzzy
matching is a simpler problem which only accepts insertions, not deletions or
substitutions.

fzy's scoring is a dynamic programming algorithm similar to
[Wagner–Fischer](https://en.wikipedia.org/wiki/Wagner%E2%80%93Fischer_algorithm)
and
[Needleman–Wunsch](https://en.wikipedia.org/wiki/Needleman%E2%80%93Wunsch_algorithm).

Dynamic programming requires the observation that the result is based on the
result of subproblems.

Fzy borrows heavily from concepts in bioinformatics to performs scoring.

Fzy builds a `n`-by-`m` matrix, where `n` is the length of the search string
and `m` the length of the candidate string. Each position `(i,j)` in the matrix
stores the score for matching the first `i` characters of the search with the
first `j` characters of the candidate.

Fzy calculates an affine gap penalty, this means simply that we assign a
constant penalty for having a gap and a linear penalty for the length of the
gap.
Inspired by the [Gotoh algorithm
(pdf)](http://www.cs.unibo.it/~dilena/LabBII/Papers/AffineGaps.pdf), fzy
computes a second `D` (for diagonal) matrix in parallel with the score matrix.
The `D` matrix computes the best score which *ends* in a match. This allows
both computation of the penalty for starting a gap and the score for a
consecutive match.

Using [this 
algorithm](https://github.com/jhawthorn/fzy/blob/master/src/match.c#L105) fzy 
is able to score based on the optimal match.

* Gaps (negative score)
  * at the start of the match
  * at the end of the match
  * within the match
* Matches (positive score)
  * consecutive
  * following a slash
  * following a space, underscore, or dash (the start of a word)
  * capital letter (the start of a CamelCase word)
  * following a dot (often a file extension)



# Other fuzzy finders

## TextMate

TextMate deserves immense credit for popularizing fuzzy finding from inside
text editors. It's influence can be found in the command-t project, various
other editors use command-t for file finding, and the 't' command in the github
web interface.

* https://github.com/textmate/textmate/blob/master/Frameworks/text/src/ranker.cc

## command-t, ctrlp-cmatcher

Command-t is a plugin first released in 2010 intending to bring TextMate's
"Go to File" feature to vim.

Anecdotally, this algorithm works very well. The recursive nature makes it a little hard to 

The wy `last_idx` is suspicious.

* https://github.com/wincent/command-t/blob/master/ruby/command-t/match.c
* https://github.com/JazzCore/ctrlp-cmatcher/blob/master/autoload/fuzzycomt.c

## Length of shortest first match: fzf
https://github.com/junegunn/fzf/blob/master/src/algo/algo.go

Fzy scores based on the size of the greedy shortest match. fzf finds its match
by the first match appearing in the candidate string. It has some cleverness to
find if there is a shorter match contained in that search, but it isn't
guaranteed to find the shortest match in the string.

Example results for the search "abc"

* <tt>**AXXBXXC**xxabc</tt>
* <tt>xxxxxxx**AXBXC**</tt>
* <tt>xxxxxxxxx**ABC**</tt>

## Length of first match: ctrlp, pick, selecta (`<= 0.0.6`)

These score based on the length of the first match in the candidate. This is
probably the simplest useful algorithm. This has the advantage that the heavy
lifting can be performed by the regex engine, which is faster than implementing
anything natively in ruby or Vim script.

## Length of shortest match: pick

Pick has a method, `min_match`, to find the absolute shortest match in a string.
This will find better results than the finders, at the expense of speed, as backtracking is required.

## selecta (latest master)
https://github.com/garybernhardt/selecta/commit/d874c99dd7f0f94225a95da06fc487b0fa5b9edc
https://github.com/garybernhardt/selecta/issues/80

Selecta doesn't compare all possible matches, but only the shortest match from the same start location.
This can lead to inconsistent results.

Example results for the search "abc"

* <tt>x**AXXXXBC**</tt>
* <tt>x**ABXC**x</tt>
* <tt>x**ABXC**xbc</tt>

The third result here should have been scored the same as the first, but the
lower scoring but shorter match is what is measured.


## others

* https://github.com/joshaven/string_score/blob/master/coffee/string_score.coffee (first match + heuristics)
* https://github.com/atom/fuzzaldrin/blob/master/src/scorer.coffee (modified version of string_score)
* https://github.com/jeancroy/fuzzaldrin-plus/blob/master/src/scorer.coffee (Smith Waterman)


# Possible fzy Algorithm Improvements

## Case sensitivity

fzy currently treats all searches as case-insensitive. However, scoring prefers
matches on uppercase letters to help find CamelCase candidates. It might be
desirable to support a case sensitive flag or "smart case" searching.

## Faster matching

Matching is currently performed using the standard lib's `strpbrk`, which has a
very simple implementation (at least in glibc).

Glibc has an extremely clever `strchr` implementation which searches the haystack
string by [word](https://en.wikipedia.org/wiki/Word_(computer_architecture)), a
4 or 8 byte `long int`, instead of by byte. It tests if a word is likely to
contain either the search char or the null terminator using bit twiddling.

A similar method could probably be written to perform to find a character in a
string case-insensitively.

* https://sourceware.org/git/?p=glibc.git;a=blob;f=string/strchr.c;h=f73891d439dcd8a08954fad4d4615acac4e0eb85;hb=HEAD

