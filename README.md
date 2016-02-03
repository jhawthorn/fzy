# fzy

A fuzzy text selector for terminals in C inspired by
[selecta](https://github.com/garybernhardt/selecta),
but with an improved [scoring algorithm](#sorting).

![](http://i.hawth.ca/u/fzy2.gif)

> It's been kind of life-changing.
> -[@graygilmore](https://github.com/graygilmore/)

## Installation

    $ make
    $ sudo make install

The `PREFIX` environment variable can be used to specify the install location,
the default is `/usr/local`.

## Usage

fzy is a drop in replacement for [selecta](https://github.com/garybernhardt/selecta), and can be used with its [usage examples](https://github.com/garybernhardt/selecta#usage-examples).

### Use with Vim

fzy can be integrated very simply in vim. There is also [fzy-vim](https://github.com/Dkendal/fzy-vim), a more fully featured vim plugin.

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

nnoremap <leader>e :call FzyCommand("find -type f", ":e")<cr>
nnoremap <leader>v :call FzyCommand("find -type f", ":vs")<cr>
nnoremap <leader>s :call FzyCommand("find -type f", ":sp")<cr>
```

Any program can be used to filter files presented through fzy. [ag (the silver searcher)](https://github.com/ggreer/the_silver_searcher) can be used to ignore files specified by `.gitignore`.

``` vim
nnoremap <leader>e :call FzyCommand("ag . --no-color -l -g ''", ":e")<cr>
nnoremap <leader>v :call FzyCommand("ag . --no-color -l -g ''", ":vs")<cr>
nnoremap <leader>s :call FzyCommand("ag . --no-color -l -g ''", ":sp")<cr>
```

## Sorting

fzy attempts to present the best matches first. The following considerations are weighted when sorting:

It prefers consecutive characters: `file` will match <tt><b>file</b></tt> over <tt><b>fil</b>t<b>e</b>r</tt>.

It prefers matching the beginning of words: `amp` is likely to match <tt><b>a</b>pp/<b>m</b>odels/<b>p</b>osts.rb</tt>.

It prefers shorter matches: `abce` matches <tt><b>abc</b>d<b>e</b>f</tt> over <tt><b>abc</b> d<b>e</b></tt>.

It prefers shorter candidates: `test` matches <tt><b>test</b>s</tt> over <tt><b>test</b>ing</b></tt>.

