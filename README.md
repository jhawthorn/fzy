![fzy](http://i.hawth.ca/u/fzy-github.svg)

**fzy** is a fast, simple fuzzy text selector for the terminal with an advanced scoring algorithm.

[Try it out online!](http://jhawthorn.github.io/fzy-demo)

![](http://i.hawth.ca/u/fzy_animated_demo.svg)

<blockquote>
It's been kind of life-changing.
-<a href="https://github.com/graygilmore/">@graygilmore</a>
</blockquote>

<blockquote>
fzy works great btw
-<a href="https://twitter.com/alexblackie/status/719297828892188672">@alexblackie</a>
</blockquote>

 [![Build Status](https://github.com/jhawthorn/fzy/workflows/CI/badge.svg)](https://github.com/jhawthorn/fzy/actions)

## Why use this over fzf, pick, selecta, ctrlp, ...?

fzy is faster and shows better results than other fuzzy finders.

Most other fuzzy matchers sort based on the length of a match. fzy tries to
find the result the user intended. It does this by favouring matches on
consecutive letters and starts of words. This allows matching using acronyms or
different parts of the path.

A gory comparison of the sorting used by fuzzy finders can be found in [ALGORITHM.md](ALGORITHM.md)

fzy is designed to be used both as an editor plugin and on the command line.
Rather than clearing the screen, fzy displays its interface directly below the current cursor position, scrolling the screen if necessary.

## Installation

**macOS**

Using Homebrew

    brew install fzy

Using MacPorts

    sudo port install fzy

**[Arch Linux](https://www.archlinux.org/packages/?sort=&q=fzy&maintainer=&flagged=)/MSYS2**: `pacman -S fzy`

**[FreeBSD](https://www.freebsd.org/cgi/ports.cgi?query=fzy&stype=all)**: `pkg install fzy`

**[Gentoo Linux](https://packages.gentoo.org/packages/app-shells/fzy)**: `emerge -av app-shells/fzy`

**[Ubuntu](https://packages.ubuntu.com/search?keywords=fzy&searchon=names&suite=bionic&section=all)/[Debian](https://packages.debian.org/search?keywords=fzy&searchon=names&suite=all&section=all)**: `apt-get install fzy`

**[pkgsrc](http://pkgsrc.se/misc/fzy) (NetBSD and others)**: `pkgin install fzy`

**[openSUSE](https://software.opensuse.org/package/fzy)**: `zypper in fzy`

### From source

    make
    sudo make install

The `PREFIX` environment variable can be used to specify the install location,
the default is `/usr/local`.

## Usage

fzy is a drop in replacement for [selecta](https://github.com/garybernhardt/selecta), and can be used with its [usage examples](https://github.com/garybernhardt/selecta#usage-examples).

### Use with Vim

fzy can be easily integrated with vim.

``` vim
function! FzyCommand(choice_command, vim_command)
  try
    let output = system(a:choice_command . " | fzy ")
  catch /Vim:Interrupt/
    " Swallow errors from ^C, allow redraw! below
  endtry
  redraw!
  if v:shell_error == 0 && !empty(output)
    exec a:vim_command . ' ' . output
  endif
endfunction

nnoremap <leader>e :call FzyCommand("find . -type f", ":e")<cr>
nnoremap <leader>v :call FzyCommand("find . -type f", ":vs")<cr>
nnoremap <leader>s :call FzyCommand("find . -type f", ":sp")<cr>
```

Any program can be used to filter files presented through fzy. [ag (the silver searcher)](https://github.com/ggreer/the_silver_searcher) can be used to ignore files specified by `.gitignore`.

``` vim
nnoremap <leader>e :call FzyCommand("ag . --silent -l -g ''", ":e")<cr>
nnoremap <leader>v :call FzyCommand("ag . --silent -l -g ''", ":vs")<cr>
nnoremap <leader>s :call FzyCommand("ag . --silent -l -g ''", ":sp")<cr>
```

## Sorting

fzy attempts to present the best matches first. The following considerations are weighted when sorting:

It prefers consecutive characters: `file` will match <tt><b>file</b></tt> over <tt><b>fil</b>t<b>e</b>r</tt>.

It prefers matching the beginning of words: `amp` is likely to match <tt><b>a</b>pp/<b>m</b>odels/<b>p</b>osts.rb</tt>.

It prefers shorter matches: `abce` matches <tt><b>abc</b>d<b>e</b>f</tt> over <tt><b>abc</b> d<b>e</b></tt>.

It prefers shorter candidates: `test` matches <tt><b>test</b>s</tt> over <tt><b>test</b>ing</b></tt>.

## See Also

* [fzy.js](https://github.com/jhawthorn/fzy.js) Javascript port


