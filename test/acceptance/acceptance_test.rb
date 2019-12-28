# coding: utf-8
require 'minitest'
require 'minitest/autorun'
require 'ttytest'

class FzyTest < Minitest::Test
  FZY_PATH = File.expand_path('../../../fzy', __FILE__)

  LEFT =  "\e[D"
  RIGHT = "\e[C"

  def test_empty_list
    @tty = interactive_fzy(input: %w[], before: "placeholder")
    @tty.assert_cursor_position(y: 1, x: 2)
    @tty.assert_matches <<~TTY
      placeholder
      >
    TTY

    @tty.send_keys('t')
    @tty.assert_cursor_position(y: 1, x: 3)
    @tty.assert_matches <<~TTY
      placeholder
      > t
    TTY

    @tty.send_keys('z')
    @tty.assert_cursor_position(y: 1, x: 4)
    @tty.assert_matches <<~TTY
      placeholder
      > tz
    TTY

    @tty.send_keys("\r")
    @tty.assert_cursor_position(y: 2, x: 0)
    @tty.assert_matches <<~TTY
      placeholder
      tz
    TTY
  end

  def test_one_item
    @tty = interactive_fzy(input: %w[test], before: "placeholder")
    @tty.assert_matches <<~TTY
      placeholder
      >
      test
    TTY
    @tty.assert_cursor_position(y: 1, x: 2)

    @tty.send_keys('t')
    @tty.assert_cursor_position(y: 1, x: 3)
    @tty.assert_matches <<~TTY
      placeholder
      > t
      test
    TTY

    @tty.send_keys('z')
    @tty.assert_cursor_position(y: 1, x: 4)
    @tty.assert_matches <<~TTY
      placeholder
      > tz
    TTY

    @tty.send_keys("\r")
    @tty.assert_cursor_position(y: 2, x: 0)
    @tty.assert_matches <<~TTY
      placeholder
      tz
    TTY
  end

  def test_two_items
    @tty = interactive_fzy(input: %w[test foo], before: "placeholder")
    @tty.assert_cursor_position(y: 1, x: 2)
    @tty.assert_matches <<~TTY
      placeholder
      >
      test
      foo
    TTY

    @tty.send_keys('t')
    @tty.assert_cursor_position(y: 1, x: 3)
    @tty.assert_matches <<~TTY
      placeholder
      > t
      test
    TTY

    @tty.send_keys('z')
    @tty.assert_cursor_position(y: 1, x: 4)
    @tty.assert_matches <<~TTY
      placeholder
      > tz
    TTY

    @tty.send_keys("\r")
    @tty.assert_matches <<~TTY
      placeholder
      tz
    TTY
    @tty.assert_cursor_position(y: 2, x: 0)
  end

  def ctrl(key)
    ((key.upcase.ord) - ('A'.ord) + 1).chr
  end

  def test_editing
    @tty = interactive_fzy(input: %w[test foo], before: "placeholder")
    @tty.assert_cursor_position(y: 1, x: 2)
    @tty.assert_matches <<~TTY
      placeholder
      >
      test
      foo
    TTY

    @tty.send_keys("foo bar baz")
    @tty.assert_cursor_position(y: 1, x: 13)
    @tty.assert_matches <<~TTY
      placeholder
      > foo bar baz
    TTY

    @tty.send_keys(ctrl('H'))
    @tty.assert_cursor_position(y: 1, x: 12)
    @tty.assert_matches <<~TTY
      placeholder
      > foo bar ba
    TTY

    @tty.send_keys(ctrl('W'))
    @tty.assert_cursor_position(y: 1, x: 10)
    @tty.assert_matches <<~TTY
      placeholder
      > foo bar
    TTY

    @tty.send_keys(ctrl('U'))
    @tty.assert_cursor_position(y: 1, x: 2)
    @tty.assert_matches <<~TTY
      placeholder
      >
      test
      foo
    TTY
  end

  def test_ctrl_d
    @tty = interactive_fzy(input: %w[foo bar])
    @tty.assert_matches ">\nfoo\nbar"

    @tty.send_keys('foo')
    @tty.assert_matches "> foo\nfoo"

    @tty.send_keys(ctrl('D'))
    @tty.assert_matches ''
    @tty.assert_cursor_position(y: 0, x: 0)
  end

  def test_ctrl_c
    @tty = interactive_fzy(input: %w[foo bar])
    @tty.assert_matches ">\nfoo\nbar"

    @tty.send_keys('foo')
    @tty.assert_matches "> foo\nfoo"

    @tty.send_keys(ctrl('C'))
    @tty.assert_matches ''
    @tty.assert_cursor_position(y: 0, x: 0)
  end

  def test_down_arrow
    @tty = interactive_fzy(input: %w[foo bar])
    @tty.assert_matches ">\nfoo\nbar"
    @tty.send_keys("\e[A\r")
    @tty.assert_matches "bar"

    @tty = interactive_fzy(input: %w[foo bar])
    @tty.assert_matches ">\nfoo\nbar"
    @tty.send_keys("\eOA\r")
    @tty.assert_matches "bar"
  end

  def test_up_arrow
    @tty = interactive_fzy(input: %w[foo bar])
    @tty.assert_matches ">\nfoo\nbar"
    @tty.send_keys("\e[A")   # first down
    @tty.send_keys("\e[B\r") # and back up
    @tty.assert_matches "foo"

    @tty = interactive_fzy(input: %w[foo bar])
    @tty.assert_matches ">\nfoo\nbar"
    @tty.send_keys("\eOA")   # first down
    @tty.send_keys("\e[B\r") # and back up
    @tty.assert_matches "foo"
  end

  def test_lines
    input10 = (1..10).map(&:to_s)
    input20 = (1..20).map(&:to_s)

    @tty = interactive_fzy(input: input10)
    @tty.assert_matches ">\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10"

    @tty = interactive_fzy(input: input20)
    @tty.assert_matches ">\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10"

    @tty = interactive_fzy(input: input10, args: "-l 5")
    @tty.assert_matches ">\n1\n2\n3\n4\n5"

    @tty = interactive_fzy(input: input10, args: "--lines=5")
    @tty.assert_matches ">\n1\n2\n3\n4\n5"
  end

  def test_prompt
    @tty = interactive_fzy
    @tty.send_keys("foo")
    @tty.assert_matches '> foo'

    @tty = interactive_fzy(args: "-p 'C:\\'")
    @tty.send_keys("foo")
    @tty.assert_matches 'C:\foo'

    @tty = interactive_fzy(args: "--prompt=\"foo bar \"")
    @tty.send_keys("baz")
    @tty.assert_matches "foo bar baz"
  end

  def test_show_scores
    expected_score = '(  inf)'
    @tty = interactive_fzy(input: %w[foo bar], args: "-s")
    @tty.send_keys('foo')
    @tty.assert_matches "> foo\n#{expected_score} foo"

    @tty = interactive_fzy(input: %w[foo bar], args: "--show-scores")
    @tty.send_keys('foo')
    @tty.assert_matches "> foo\n#{expected_score} foo"

    expected_score = '( 0.89)'
    @tty = interactive_fzy(input: %w[foo bar], args: "-s")
    @tty.send_keys('f')
    @tty.assert_matches "> f\n#{expected_score} foo"
  end

  def test_large_input
    @tty = TTYtest.new_terminal(%{seq 100000 | #{FZY_PATH} -l 3})
    @tty.send_keys('34')
    @tty.assert_matches "> 34\n34\n340\n341"

    @tty.send_keys('5')
    @tty.assert_matches "> 345\n345\n3450\n3451"

    @tty.send_keys('z')
    @tty.assert_matches "> 345z"
  end

  def test_worker_count
    @tty = interactive_fzy(input: %w[foo bar], args: "-j1")
    @tty.send_keys('foo')
    @tty.assert_matches "> foo\nfoo"

    @tty = TTYtest.new_terminal(%{seq 100000 | #{FZY_PATH} -j1 -l3})
    @tty.send_keys('34')
    @tty.assert_matches "> 34\n34\n340\n341"

    @tty = TTYtest.new_terminal(%{seq 100000 | #{FZY_PATH} -j200 -l3})
    @tty.send_keys('34')
    @tty.assert_matches "> 34\n34\n340\n341"
  end

  def test_initial_query
    @tty = interactive_fzy(input: %w[foo bar], args: "-q fo")
    @tty.assert_matches "> fo\nfoo"
    @tty.send_keys("o")
    @tty.assert_matches "> foo\nfoo"
    @tty.send_keys("o")
    @tty.assert_matches "> fooo"

    @tty = interactive_fzy(input: %w[foo bar], args: "-q asdf")
    @tty.assert_matches "> asdf"
  end

  def test_non_interactive
    @tty = interactive_fzy(input: %w[foo bar], args: "-e foo", before: "before", after: "after")
    @tty.assert_matches "before\nfoo\nafter"
  end

  def test_moving_text_cursor
    @tty = interactive_fzy(input: %w[foo bar])
    @tty.send_keys("br")
    @tty.assert_matches "> br\nbar"
    @tty.assert_cursor_position(y: 0, x: 4)

    @tty.send_keys(LEFT)
    @tty.assert_cursor_position(y: 0, x: 3)
    @tty.assert_matches "> br\nbar"
    @tty.send_keys("a")
    @tty.assert_cursor_position(y: 0, x: 4)
    @tty.assert_matches "> bar\nbar"

    @tty.send_keys(ctrl("A")) # Ctrl-A
    @tty.assert_cursor_position(y: 0, x: 2)
    @tty.assert_matches "> bar\nbar"
    @tty.send_keys("foo")
    @tty.assert_cursor_position(y: 0, x: 5)
    @tty.assert_matches "> foobar"

    @tty.send_keys(ctrl("E")) # Ctrl-E
    @tty.assert_cursor_position(y: 0, x: 8)
    @tty.assert_matches "> foobar"
    @tty.send_keys("baz") # Ctrl-E
    @tty.assert_cursor_position(y: 0, x: 11)
    @tty.assert_matches "> foobarbaz"
  end

  # More info;
  # https://github.com/jhawthorn/fzy/issues/42
  # https://cirw.in/blog/bracketed-paste
  def test_bracketed_paste_characters
    @tty = interactive_fzy(input: %w[foo bar])
    @tty.assert_matches ">\nfoo\nbar"
    @tty.send_keys("\e[200~foo\e[201~")
    @tty.assert_matches "> foo\nfoo"
  end

  # https://github.com/jhawthorn/fzy/issues/81
  def test_slow_stdin_fast_user
    @tty = TTYtest.new_terminal(%{(sleep 0.5; echo aa; echo bc; echo bd) | #{FZY_PATH}})

    # Before input has all come in, but wait for fzy to at least start
    sleep 0.1

    @tty.send_keys("b\r")
    @tty.assert_matches "bc"
  end

  def test_unicode
    @tty = interactive_fzy(input: %w[English Français 日本語])
    @tty.assert_matches <<~TTY
      >
      English
      Français
      日本語
    TTY
    @tty.assert_cursor_position(y: 0, x: 2)

    @tty.send_keys("ç")
    @tty.assert_matches <<~TTY
      > ç
      Français
    TTY
    @tty.assert_cursor_position(y: 0, x: 3)

    @tty.send_keys("\r")
    @tty.assert_matches "Français"
  end

  def test_unicode_backspace
    @tty = interactive_fzy
    @tty.send_keys "Français"
    @tty.assert_matches "> Français"
    @tty.assert_cursor_position(y: 0, x: 10)

    @tty.send_keys(ctrl('H') * 3)
    @tty.assert_matches "> Franç"
    @tty.assert_cursor_position(y: 0, x: 7)

    @tty.send_keys(ctrl('H'))
    @tty.assert_matches "> Fran"
    @tty.assert_cursor_position(y: 0, x: 6)

    @tty.send_keys('ce')
    @tty.assert_matches "> France"

    @tty = interactive_fzy
    @tty.send_keys "日本語"
    @tty.assert_matches "> 日本語"
    @tty.send_keys(ctrl('H'))
    @tty.assert_matches "> 日本"
    @tty.send_keys(ctrl('H'))
    @tty.assert_matches "> 日"
    @tty.send_keys(ctrl('H'))
    @tty.assert_matches "> "
    @tty.assert_cursor_position(y: 0, x: 2)
  end

  def test_unicode_delete_word
    @tty = interactive_fzy
    @tty.send_keys "Je parle Français"
    @tty.assert_matches "> Je parle Français"
    @tty.assert_cursor_position(y: 0, x: 19)

    @tty.send_keys(ctrl('W'))
    @tty.assert_matches "> Je parle"
    @tty.assert_cursor_position(y: 0, x: 11)

    @tty = interactive_fzy
    @tty.send_keys "日本語"
    @tty.assert_matches "> 日本語"
    @tty.send_keys(ctrl('W'))
    @tty.assert_matches "> "
    @tty.assert_cursor_position(y: 0, x: 2)
  end

  def test_unicode_cursor_movement
    @tty = interactive_fzy
    @tty.send_keys "Français"
    @tty.assert_cursor_position(y: 0, x: 10)

    @tty.send_keys(LEFT*5)
    @tty.assert_cursor_position(y: 0, x: 5)

    @tty.send_keys(RIGHT*3)
    @tty.assert_cursor_position(y: 0, x: 8)

    @tty = interactive_fzy
    @tty.send_keys "日本語"
    @tty.assert_matches "> 日本語"
    @tty.assert_cursor_position(y: 0, x: 8)
    @tty.send_keys(LEFT)
    @tty.assert_cursor_position(y: 0, x: 6)
    @tty.send_keys(LEFT)
    @tty.assert_cursor_position(y: 0, x: 4)
    @tty.send_keys(LEFT)
    @tty.assert_cursor_position(y: 0, x: 2)
    @tty.send_keys(LEFT)
    @tty.assert_cursor_position(y: 0, x: 2)
    @tty.send_keys(RIGHT*3)
    @tty.assert_cursor_position(y: 0, x: 8)
    @tty.send_keys(RIGHT)
    @tty.assert_cursor_position(y: 0, x: 8)
  end

  def test_long_strings
    ascii = "LongStringOfText" * 6
    unicode = "ＬｏｎｇＳｔｒｉｎｇＯｆＴｅｘｔ" * 3

    @tty = interactive_fzy(input: [ascii, unicode])
    @tty.assert_matches <<~TTY
      >
      LongStringOfTextLongStringOfTextLongStringOfTextLongStringOfTextLongStringOfText
      ＬｏｎｇＳｔｒｉｎｇＯｆＴｅｘｔＬｏｎｇＳｔｒｉｎｇＯｆＴｅｘｔＬｏｎｇＳｔｒｉ
    TTY
  end

  def test_show_info
    @tty = interactive_fzy(input: %w[foo bar baz], args: "-i")
    @tty.assert_matches ">\n[3/3]\nfoo\nbar\nbaz"
    @tty.send_keys("ba")
    @tty.assert_matches "> ba\n[2/3]\nbar\nbaz"
    @tty.send_keys("q")
    @tty.assert_matches "> baq\n[0/3]"
  end

  def test_help
    @tty = TTYtest.new_terminal(%{#{FZY_PATH} --help})
    @tty.assert_matches <<TTY
Usage: fzy [OPTION]...
 -l, --lines=LINES        Specify how many lines of results to show (default 10)
 -p, --prompt=PROMPT      Input prompt (default '> ')
 -q, --query=QUERY        Use QUERY as the initial search string
 -e, --show-matches=QUERY Output the sorted matches of QUERY
 -t, --tty=TTY            Specify file to use as TTY device (default /dev/tty)
 -s, --show-scores        Show the scores of each match
 -0, --read-null          Read input delimited by ASCII NUL characters
 -j, --workers NUM        Use NUM workers for searching. (default is # of CPUs)
 -i, --show-info          Show selection info line
 -h, --help     Display this help and exit
 -v, --version  Output version information and exit
TTY
  end

  private

  def interactive_fzy(input: [], before: nil, after: nil, args: "")
    cmd = []
    cmd << %{echo "#{before}"} if before
    cmd << %{printf "#{input.join("\\n")}" | #{FZY_PATH} #{args}}
    cmd << %{echo "#{after}"} if after
    cmd = cmd.join("; ")
    TTYtest.new_terminal(cmd)
  end
end
