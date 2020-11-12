#!/usr/bin/env ruby

# SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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
set -e
cat deinstall.sh
touch #{@tmpdir}/deinstall.sh-run
    DEINSTALL_SH
    File.write("#{archive_dir}/install.sh", <<-INSTALL_SH)
#!/bin/sh
set -e
cat install.sh
touch #{@tmpdir}/install.sh-run
    INSTALL_SH

    assert(system('servicemenuinstaller', 'uninstall', archive_base))

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

    assert(system('servicemenuinstaller', 'uninstall', archive_base))

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

    refute(system('servicemenuinstaller', 'uninstall', archive_base))

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

    assert(system('servicemenuinstaller', 'uninstall', downloaded_file))

    assert_path_exist(downloaded_file)
    assert_path_not_exist(installed_file)
  end
end
