class ObjectTest < MTest::Unit::TestCase
  def test_raise_no_args
    assert_raise(ArgumentError) do
      Strptime.new
    end
  end

  def test_instance_class
    pr = Strptime.new("%Y")
    assert_equal(pr.class, Strptime)
  end

  def test_parse_Y
    pr = Strptime.new("%Y")
    assert_equal(pr.exec("2015").year, 2015)
    assert_equal(pr.exec("2025").year, 2025)
  end

  def test_parse_y
    pr = Strptime.new("%y")
    assert_equal(pr.exec("15").year, 2015)
    assert_equal(pr.exec("25").year, 2025)
    assert_equal(pr.exec("70").year, 1970)
    assert_equal(pr.exec("68").year, 2068)
    assert_equal(pr.exec("69").year, 1969)
  end

  def test_parse_m
    pr = Strptime.new("%m")
    assert_equal(pr.exec("12").mon, 12)
    assert_equal(pr.exec("3").mon, 3)
  end

  def test_parse_d
    assert_equal(Strptime.new("%d").exec("28").utc.mday, 28)
    assert_equal(Strptime.new(" %d").exec(" 28").mday, 28)
  end

  def test_parse_B
    pr = Strptime.new("%B")
    h = pr.exec("May")
    assert_equal(h.mon, 5)
    h = pr.exec("January")
    assert_equal(h.mon, 1)
  end

  def test_parse_H
    pr = Strptime.new("%H")
    h = pr.exec("01").utc
    assert_equal(h.hour, 1)
    h = pr.exec("9").utc
    assert_equal(h.hour, 9)
    h = pr.exec("23").utc
    assert_equal(h.hour, 23)
  end

  def test_parse_M
    pr = Strptime.new("%M")
    h = pr.exec("00")
    assert_equal(h.min, 0)
    h = pr.exec("59")
    assert_equal(h.min, 59)
  end

  def test_parse_d_and_others
    pr = Strptime.new("%d")
    assert_equal(pr.exec("10").utc.year, Time.now.year)
    assert_equal(pr.exec("10").utc.month, Time.now.month)
    assert_equal(pr.exec("10").day, 10)
    assert_equal(pr.exec("10").utc.hour, 0)
    assert_equal(pr.exec("10").min, 0)
    assert_equal(pr.exec("10").sec, 0)
  end

  def test_parse_S
    pr = Strptime.new("%S")
    h = pr.exec("31")
    assert_equal(h.sec, 31)
    pr = Strptime.new("%S")
    h = pr.exec("59")
    assert_equal(h.sec, 59)
    h = pr.exec("60")
    assert_equal(h.sec, 0)
  end

  def test_parse_Y_to_z
    pr = Strptime.new("%Y%m%d%H%M%S%z")
    assert_equal(pr.exec("20150610102415+0").to_i, 1433931855)
    assert_equal(pr.exec("20150610102415+9").to_i, 1433931855-9*3600)
    assert_equal(pr.exec("20150610102415+09").to_i, 1433931855-9*3600)
    assert_equal(pr.exec("20150610102415+09:00").to_i, 1433931855-9*3600)
    assert_equal(pr.exec("20150610102415+09:0").to_i, 1433931855-9*3600)
    assert_equal(pr.exec("20150610102415+0900").to_i, 1433931855-9*3600)
    assert_equal(pr.exec("20150610102415+090").to_i, 1433931855-9*3600)
    assert_equal(pr.exec("20150610102415-0102").to_i, 1433931855+1*3600+2*60)
    assert_equal(pr.exec("20150610102415-1200").to_i, 1433931855+12*3600)
    assert_equal(pr.exec("20150610102415+1400").to_i, 1433931855-14*3600)
    assert_equal(pr.exec("20150610102415-1200").to_i, 1433931855+12*3600)
  end

  def test_parse_Y_to_z_with_symbol
    pr = Strptime.new("%Y-%m-%d %H:%M:%S")
    assert_equal(pr.exec("2015-06-11 10:24:15").year, 2015)
    assert_equal(pr.exec("2015-06-11 10:24:15").month, 6)
    assert_equal(pr.exec("2015-06-11 10:24:15").day, 11)
    assert_equal(pr.exec("2015-06-11 10:24:15").utc.hour, 10)
    assert_equal(pr.exec("2015-06-11 10:24:15").min, 24)
    assert_equal(pr.exec("2015-06-11 10:24:15").sec, 15)
  end

  def test_parse_S_and_z
    pr = Strptime.new("%S%z")
    assert_equal(pr.exec("12-03:00").year, pr.exec("12-03:00").year)
    assert_equal(pr.exec("12-03:00").month, Time.now.localtime("-03:00").month)
    assert_equal(pr.exec("12-03:00").day, Time.now.localtime("-03:00").day)
    assert_equal(pr.exec("12-03:00").hour, Time.now.localtime("-03:00").hour)
    assert_equal(pr.exec("12-03:00").min, Time.now.localtime("-03:00").min)
    assert_equal(pr.exec("12-03:00").sec, 12)

    assert_equal(pr.exec("12+09:00").year, Time.now.localtime("+09:00").year)
    assert_equal(pr.exec("12+09:00").month, Time.now.localtime("+09:00").month)
    assert_equal(pr.exec("12+09:00").day, Time.now.localtime("+09:00").day)
    assert_equal(pr.exec("12+09:00").hour, Time.now.localtime("+09:00").hour)
    assert_equal(pr.exec("12+09:00").min, Time.now.localtime("+09:00").min)
    assert_equal(pr.exec("12+09:00").sec, 12)

    assert_equal(pr.exec("12+11:00").year, Time.now.localtime("+11:00").year)
    assert_equal(pr.exec("12+11:00").month, Time.now.localtime("+11:00").month)
    assert_equal(pr.exec("12+11:00").day, Time.now.localtime("+11:00").day)
    assert_equal(pr.exec("12+11:00").hour, Time.now.localtime("+11:00").hour)
    assert_equal(pr.exec("12+11:00").min, Time.now.localtime("+11:00").min)
    assert_equal(pr.exec("12+11:00").sec, 12)
  end

  def test_parse_Ymdz
    pr = Strptime.new('%Y%m%d %z')
    assert_equal(pr.exec('20010203 -0200').year, 2001)
    assert_equal(pr.exec('20010203 -0200').mon, 2)
    assert_equal(pr.exec('20010203 -0200').day, 3)
    assert_equal(pr.exec('20010203 -0200').utc.hour, 2)
    assert_equal(pr.exec('20010203 -0200').min, 0)
    assert_equal(pr.exec('20010203 -0200').sec, 0)
  end

  def test_raise_wrong_args
    assert_raise(RuntimeError) do
      Strptime.new('%A')
    end
  end
end

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
    assert_raise(ArgumentError) do
      Strptime.new('%Y-%m-%d %H:%M:%S.N %z').exec('2017-10-06 13:15:30.0 +00:00')
    end
  end

  def test_raise_execi
    assert_raise(ArgumentError) do
      Strptime.new('%Y-%m-%d %H:%M:%S.N %z').execi('2017-10-06 13:15:30.0 +00:00')
    end
  end
end

if __FILE__ == $0
  MTest::Unit.new.run
end