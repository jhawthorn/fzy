require 'minitest'
require 'minitest/autorun'
require 'ttytest'

class FzyTest < Minitest::Test
  FZY_PATH = File.expand_path('../../../fzy', __FILE__)

  def setup
    # fzy is fast.
    # This is never hit in a (passing) test suite, but helps speed up development
    TTYtest.default_max_wait_time = 0.2
  end

  def test_empty_list
    @tty = TTYtest.new_terminal(%{echo placeholder;echo -n "" | #{FZY_PATH}})
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
    @tty = TTYtest.new_terminal(%{echo placeholder;echo -n "test" | #{FZY_PATH}})
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
    @tty = TTYtest.new_terminal(%{echo placeholder;echo -n "test\nfoo" | #{FZY_PATH}})
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
    @tty = TTYtest.new_terminal(%{echo placeholder;echo -n "test\nfoo" | #{FZY_PATH}})
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
    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH}})
    @tty.assert_matches ">\nfoo\nbar"

    @tty.send_keys('foo')
    @tty.assert_matches "> foo\nfoo"

    @tty.send_keys(ctrl('D'))
    @tty.assert_matches ''
    @tty.assert_cursor_position(y: 0, x: 0)
  end

  def test_ctrl_c
    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH}})
    @tty.assert_matches ">\nfoo\nbar"

    @tty.send_keys('foo')
    @tty.assert_matches "> foo\nfoo"

    @tty.send_keys(ctrl('C'))
    @tty.assert_matches ''
    @tty.assert_cursor_position(y: 0, x: 0)
  end

  def test_down_arrow
    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH}})
    @tty.assert_matches ">\nfoo\nbar"
    @tty.send_keys("\e[A\r")
    @tty.assert_matches "bar"

    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH}})
    @tty.assert_matches ">\nfoo\nbar"
    @tty.send_keys("\eOA\r")
    @tty.assert_matches "bar"
  end

  def test_up_arrow
    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH}})
    @tty.assert_matches ">\nfoo\nbar"
    @tty.send_keys("\e[A")   # first down
    @tty.send_keys("\e[B\r") # and back up
    @tty.assert_matches "foo"

    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH}})
    @tty.assert_matches ">\nfoo\nbar"
    @tty.send_keys("\eOA")   # first down
    @tty.send_keys("\e[B\r") # and back up
    @tty.assert_matches "foo"
  end

  def test_lines
    @tty = TTYtest.new_terminal(%{seq 10 | #{FZY_PATH}})
    @tty.assert_matches ">\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10"

    @tty = TTYtest.new_terminal(%{seq 20 | #{FZY_PATH}})
    @tty.assert_matches ">\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10"

    @tty = TTYtest.new_terminal(%{seq 10 | #{FZY_PATH} -l 5})
    @tty.assert_matches ">\n1\n2\n3\n4\n5"

    @tty = TTYtest.new_terminal(%{seq 10 | #{FZY_PATH} --lines=5})
    @tty.assert_matches ">\n1\n2\n3\n4\n5"
  end

  def test_prompt
    @tty = TTYtest.new_terminal(%{echo -n "" | #{FZY_PATH}})
    @tty.send_keys("foo")
    @tty.assert_matches '> foo'

    @tty = TTYtest.new_terminal(%{echo -n "" | #{FZY_PATH} -p 'C:\\'})
    @tty.send_keys("foo")
    @tty.assert_matches 'C:\foo'

    @tty = TTYtest.new_terminal(%{echo -n "" | #{FZY_PATH} --prompt="foo bar "})
    @tty.send_keys("baz")
    @tty.assert_matches "foo bar baz"
  end

  def test_show_scores
    expected_score = '(  inf)'
    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH} -s})
    @tty.send_keys('foo')
    @tty.assert_matches "> foo\n#{expected_score} foo"

    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH} --show-scores})
    @tty.send_keys('foo')
    @tty.assert_matches "> foo\n#{expected_score} foo"

    expected_score = '( 0.89)'
    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH} -s})
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
    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH} -j1})
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
    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH} -q fo})
    @tty.assert_matches "> fo\nfoo"
    @tty.send_keys("o")
    @tty.assert_matches "> foo\nfoo"
    @tty.send_keys("o")
    @tty.assert_matches "> fooo"

    @tty = TTYtest.new_terminal(%{echo -n "foo\nbar" | #{FZY_PATH} -q asdf})
    @tty.assert_matches "> asdf"
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
 -j, --workers NUM        Use NUM workers for searching. (default is # of CPUs)
 -h, --help     Display this help and exit
 -v, --version  Output version information and exit
TTY
  end
end
