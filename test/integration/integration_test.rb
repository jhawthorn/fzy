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
end
