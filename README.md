# mruby-fast-strptime

## install by mrbgems
```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :github => 'naritta/mruby-fast-strptime'
end
```

## Example
```ruby
parser = Strptime.new('%Y-%m-%dT%H:%M:%S%z')
parser.exec('2015-12-25T12:34:56+09') #=> 2015-12-25 12:34:56 +09:00
parser.execi('2015-12-25T12:34:56+09') #=> 1451014496
```

## test
tests can be executed with mruby-mtest
```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :github => 'naritta/mruby-fast-strptime'
    conf.gem :github => 'iij/mruby-mtest'
end
```
Then build mruby and run tests

```
$ mruby test/mruby-fast-strptime.rb
```

## License

BSD