
class ExecTest < MTest::Unit::TestCase
  def test_year
    time = Strptime.new('%Y').exec('2015')
    assert_equal(time.year, 2015)
  end

  def test_year_to_mon
    time = Strptime.new('%Y-%m').exec('2015-12')
    assert_equal(time.year, 2015)
    assert_equal(time.mon, 12)
  end

  def test_year_to_day
    time = Strptime.new('%Y-%m-%d').exec('2015-12-25')
    assert_equal(time.year, 2015)
    assert_equal(time.mon, 12)
    assert_equal(time.day, 25)
  end

  def test_year_to_hour
    time = Strptime.new('%Y-%m-%dT%H').exec('2015-12-25T12').utc
    assert_equal(time.year, 2015)
    assert_equal(time.mon, 12)
    assert_equal(time.day, 25)
    assert_equal(time.hour, 12)
  end

  def test_year_to_min
    time = Strptime.new('%Y-%m-%dT%H:%M').exec('2015-12-25T12:34').utc
    assert_equal(time.year, 2015)
    assert_equal(time.mon, 12)
    assert_equal(time.day, 25)
    assert_equal(time.hour, 12)
    assert_equal(time.min, 34)
  end

  def test_year_to_sec
    time = Strptime.new('%Y-%m-%dT%H:%M:%S').exec('2015-12-25T12:34:56').utc
    assert_equal(time.year, 2015)
    assert_equal(time.mon, 12)
    assert_equal(time.day, 25)
    assert_equal(time.hour, 12)
    assert_equal(time.min, 34)
    assert_equal(time.sec, 56)
  end
end

class ExeciTest < MTest::Unit::TestCase
  def test_to_i
    time_i = Strptime.new('%Y-%m-%dT%H:%M:%S').execi('2015-12-25T12:34:56')
    assert_equal(time_i, 1451046896)
  end

  def test_to_i_with_timezone
    time_i = Strptime.new('%Y-%m-%dT%H:%M:%S%z').execi('2015-12-25T12:34:56+09:00')
    assert_equal(time_i, 1451014496)
  end
end

class UnmatchPatternTest < MTest::Unit::TestCase
  def test_raise_exec
    assert_raise(RuntimeError) do
      Strptime.new('%Y-%m-%d %H:%M:%S.N %z').exec('2017-10-06 13:15:30.0 +00:00')
    end
  end

  def test_raise_execi
    assert_raise(RuntimeError) do
      Strptime.new('%Y-%m-%d %H:%M:%S.N %z').execi('2017-10-06 13:15:30.0 +00:00')
    end
  end
end

if __FILE__ == $0
  MTest::Unit.new.run
end