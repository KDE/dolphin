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

require 'test/unit'
