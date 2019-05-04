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

class ServiceMenuInstallationTest < Test::Unit::TestCase
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

  def test_run_install
    service_dir = File.join(Dir.pwd, 'share/servicemenu-download')
    FileUtils.mkpath(service_dir)
    archive = "#{service_dir}/foo.tar"

    archive_dir = 'foo' # relative so tar cf is relative without fuzz
    FileUtils.mkpath(archive_dir)
    File.write("#{archive_dir}/install-it.sh", <<-INSTALL_IT_SH)
#!/bin/sh
touch #{@tmpdir}/install-it.sh-run
INSTALL_IT_SH
    File.write("#{archive_dir}/install.sh", <<-INSTALL_SH)
#!/bin/sh
touch #{@tmpdir}/install.sh-run
    INSTALL_SH
    assert(system('tar', '-cf', archive, archive_dir))

    assert(covered_system('servicemenuinstallation', archive))

    tar_dir = "#{service_dir}/foo.tar-dir"
    tar_extract_dir = "#{service_dir}/foo.tar-dir/foo"
    assert_path_exist(tar_dir)
    assert_path_exist(tar_extract_dir)
    assert_path_exist("#{tar_extract_dir}/install-it.sh")
    assert_path_exist("#{tar_extract_dir}/install.sh")
  end

  def test_run_install_with_arg
    service_dir = File.join(Dir.pwd, 'share/servicemenu-download')
    FileUtils.mkpath(service_dir)
    archive = "#{service_dir}/foo.tar"

    archive_dir = 'foo' # relative so tar cf is relative without fuzz
    FileUtils.mkpath(archive_dir)
    File.write("#{archive_dir}/install.sh", <<-INSTALL_SH)
#!/bin/sh
if [ "$@" = "--install" ]; then
  touch #{@tmpdir}/install.sh-run
  exit 0
fi
exit 1
    INSTALL_SH
    assert(system('tar', '-cf', archive, archive_dir))

    assert(covered_system('servicemenuinstallation', archive))

    tar_dir = "#{service_dir}/foo.tar-dir"
    tar_extract_dir = "#{service_dir}/foo.tar-dir/foo"
    assert_path_exist(tar_dir)
    assert_path_exist(tar_extract_dir)
    assert_path_not_exist("#{tar_extract_dir}/install-it.sh")
    assert_path_exist("#{tar_extract_dir}/install.sh")
  end

  def test_run_fail
    service_dir = File.join(Dir.pwd, 'share/servicemenu-download')
    FileUtils.mkpath(service_dir)
    archive = "#{service_dir}/foo.tar"

    archive_dir = 'foo' # relative so tar cf is relative without fuzz
    FileUtils.mkpath(archive_dir)
    assert(system('tar', '-cf', archive, archive_dir))

    refute(covered_system('servicemenuinstallation', archive))
  end

  def test_run_desktop
    service_dir = File.join(Dir.pwd, 'share/servicemenu-download')
    downloaded_file = "#{service_dir}/foo.desktop"
    FileUtils.mkpath(service_dir)
    FileUtils.touch(downloaded_file)

    installed_file = "#{ENV['XDG_DATA_HOME']}/kservices5/ServiceMenus/foo.desktop"

    assert(covered_system('servicemenuinstallation', downloaded_file))

    assert_path_exist(downloaded_file)
    assert_path_exist(installed_file)
  end
end
