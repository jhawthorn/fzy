require "minitest/autorun"

# Largely borrowed from selecta
describe "score" do
  def score(candidate, query)
    # FIXME: should escape this properly
    ret = `./testscore '#{query}' '#{candidate}'`
    ret.to_f unless ret.empty?
  end

  def assert_unmatched(candidate, query)
    assert_equal nil, score(candidate, query)
  end

  def assert_matched(candidate, query)
    assert_operator 0, :<=, score(candidate, query)
  end

  it "scores 1 when the query is empty" do
    assert_equal 1, score("a", "")
  end

  it "scores 0 when the choice is empty" do
    assert_unmatched "", "a"
  end

  it "scores 1 when exact match" do
    assert_equal 1, score("a", "a")
  end

  it "scores 0 when the query is longer than the choice" do
    assert_unmatched "short", "longer"
  end

  it "scores 0 when the query doesn't match at all" do
    assert_unmatched "a", "b"
  end

  it "scores 0 when only a prefix of the query matches" do
    assert_unmatched "ab", "ac"
  end

  it "scores greater than 0 when it matches" do
    assert_matched "a", "a"
    assert_matched "ab", "a"
    assert_matched "ba", "a"
    assert_matched "bab", "a"
    assert_matched "bababababab", "aaaaa"
  end

  it "prefers start of words" do
    assert_operator score("app/models/foo", "amo"), :<, score("app/models/order", "amo")
  end
end
