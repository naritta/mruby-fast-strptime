MRuby::Build.new do |conf|
  toolchain :gcc
  enable_debug

  gembox 'full-core'
  gem :github => 'iij/mruby-mtest'
  gem File.expand_path(File.dirname(__FILE__))
end
