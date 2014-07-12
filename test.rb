require "minitest/autorun"

# Largely borrowed from selecta
describe "score" do
  def score(candidate, query)
    # FIXME: should escape this properly
    `./testscore '#{query}' '#{candidate}'`.to_f
  end

  it "scores 1 when the query is empty" do
    assert_equal 1, score("a", "")
  end

  it "scores 0 when the choice is empty" do
    assert_equal 0, score("", "a")
  end

  it "scores 1 when exact match" do
    assert_equal 1, score("a", "a")
  end

  it "scores 0 when the query is longer than the choice" do
    assert_equal 0, score("short", "longer")
  end

  it "scores 0 when the query doesn't match at all" do
    assert_equal 0, score("a", "b")
  end

  it "scores 0 when only a prefix of the query matches" do
    assert_equal 0, score("ab", "ac")
  end

  it "scores greater than 0 when it matches" do
    assert_operator 0, :<, score("a", "a")
    assert_operator 0, :<, score("ab", "a")
    assert_operator 0, :<, score("ba", "a")
    assert_operator 0, :<, score("bab", "a")
    assert_operator 0, :<, score("babababab", "aaaa")
  end
end
