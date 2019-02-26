#!/usr/bin/env ruby

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

require_relative 'test_helper'

require 'tmpdir'

class ServiceMenuDeinstallationTest < Test::Unit::TestCase
  def setup
    @tmpdir = Dir.mktmpdir("dolphintest-#{self.class.to_s.tr(':', '_')}")
    @pwdir = Dir.pwd
    Dir.chdir(@tmpdir)

    ENV['XDG_DATA_HOME'] = File.join(@tmpdir, 'data')
  end

  def teardown
    Dir.chdir(@pwdir)
    FileUtils.rm_rf(@tmpdir)

    ENV.delete('XDG_DATA_HOME')
  end

  def test_run_deinstall
    service_dir = File.join(Dir.pwd, 'share/servicemenu-download')
    archive_base = "#{service_dir}/foo.zip"
    archive_dir = "#{archive_base}-dir/foo-1.1/"
    FileUtils.mkpath(archive_dir)
    File.write("#{archive_dir}/deinstall.sh", <<-DEINSTALL_SH)
#!/bin/sh
touch #{@tmpdir}/deinstall.sh-run
    DEINSTALL_SH
    File.write("#{archive_dir}/install.sh", <<-INSTALL_SH)
#!/bin/sh
touch #{@tmpdir}/install.sh-run
    INSTALL_SH

    assert(covered_system('servicemenudeinstallation', archive_base))

    # deinstaller should be run
    # installer should not be run
    # archive_dir should have been correctly removed

    assert_path_exist('deinstall.sh-run')
    assert_path_not_exist('install.sh-run')
    assert_path_not_exist(archive_dir)
  end

  def test_run_install_with_arg
    service_dir = File.join(Dir.pwd, 'share/servicemenu-download')
    archive_base = "#{service_dir}/foo.zip"
    archive_dir = "#{archive_base}-dir/foo-1.1/"
    FileUtils.mkpath(archive_dir)

    File.write("#{archive_dir}/install.sh", <<-INSTALL_SH)
#!/bin/sh
if [ "$@" = "--uninstall" ]; then
  touch #{@tmpdir}/install.sh-run
  exit 0
fi
exit 1
    INSTALL_SH

    assert(covered_system('servicemenudeinstallation', archive_base))

    assert_path_not_exist('deinstall.sh-run')
    assert_path_exist('install.sh-run')
    assert_path_not_exist(archive_dir)
  end

  # no scripts in sight
  def test_run_fail
    service_dir = File.join(Dir.pwd, 'share/servicemenu-download')
    archive_base = "#{service_dir}/foo.zip"
    archive_dir = "#{archive_base}-dir/foo-1.1/"
    FileUtils.mkpath(archive_dir)

    refute(covered_system('servicemenudeinstallation', archive_base))

    # I am unsure if deinstallation really should keep the files around. But
    # that's how it behaved originally so it's supposedly intentional
    #   - sitter, 2019
    assert_path_exist(archive_dir)
  end

  # For desktop files things are a bit special. There is one in .local/share/servicemenu-download
  # and another in the actual ServiceMenus dir. The latter gets removed by the
  # script, the former by KNS.
  def test_run_desktop
    service_dir = File.join(Dir.pwd, 'share/servicemenu-download')
    downloaded_file = "#{service_dir}/foo.desktop"
    FileUtils.mkpath(service_dir)
    FileUtils.touch(downloaded_file)

    menu_dir = "#{ENV['XDG_DATA_HOME']}/kservices5/ServiceMenus/"
    installed_file = "#{menu_dir}/foo.desktop"
    FileUtils.mkpath(menu_dir)
    FileUtils.touch(installed_file)

    assert(covered_system('servicemenudeinstallation', downloaded_file))

    assert_path_exist(downloaded_file)
    assert_path_not_exist(installed_file)
  end
end
