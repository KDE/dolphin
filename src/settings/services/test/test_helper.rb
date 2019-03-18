# Copyright (C) 2019 Harald Sitter <sitter@kde.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

GLOBAL_COVERAGE_ROOT = File.dirname(__dir__) # ../

# Simplecov is a bit meh and expects src and coverage to be under the
# same root. Since we get run through cmake that assumption absolutely
# doesn't hold true, so we'll need to figure out the coverage_dir relative
# to the root and the root must always be the source :/
# The relativity only works because internally the path gets expanded, this
# isn't fully reliable, but oh well...
# https://github.com/colszowka/simplecov/issues/716
GLOBAL_COVERAGE_DIR = begin
  require 'pathname'
  src_path = Pathname.new(GLOBAL_COVERAGE_ROOT)
  coverage_path = Pathname.new(File.join(Dir.pwd, 'coverage'))
  coverage_path.relative_path_from(src_path).to_s
end

begin
  require 'simplecov'

  SimpleCov.formatter = SimpleCov::Formatter::MultiFormatter.new(
    [
      SimpleCov::Formatter::HTMLFormatter
    ]
  )

  SimpleCov.start do
    root GLOBAL_COVERAGE_ROOT
    coverage_dir GLOBAL_COVERAGE_DIR
  end
rescue LoadError
  warn 'SimpleCov not loaded'
end

# FIXME: add coverage report for jenkins?

$LOAD_PATH.unshift(File.absolute_path('../', __dir__)) # ../

def __test_method_name__
  return @method_name if defined?(:@method_name)
  index = 0
  caller = ''
  until caller.start_with?('test_')
    caller = caller_locations(index, 1)[0].label
    index += 1
  end
  caller
end

# system() variant which sets up merge-coverage. simplecov supports merging
# of multiple coverage sets. we use this to get coverage metrics on the
# binaries without having to refactor the script into runnable classes.
def covered_system(cmd, *argv)
  pid = fork do
    Kernel.module_exec do
      alias_method(:real_system, :system)
      define_method(:system) do |*args|
        return true if args.include?('kdialog') # disable kdialog call
        real_system(*args)
      end
    end

    begin
      require 'simplecov'
      SimpleCov.start do
        root GLOBAL_COVERAGE_ROOT
        coverage_dir GLOBAL_COVERAGE_DIR
        command_name "#{cmd}_#{__test_method_name__}"
      end
    rescue LoadError
      warn 'SimpleCov not loaded'
    end

    ARGV.replace(argv)
    load "#{__dir__}/../#{cmd}"
    puts 'all good, fork ending!'
    exit 0
  end
  waitedpid, status = Process.waitpid2(pid)
  assert_equal(pid, waitedpid)
  status.success? # behave like system and return the success only
end

require 'test/unit'
