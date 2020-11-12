# SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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
