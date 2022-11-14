#compdef dolphin

# SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>
#
# SPDX-License-Identifier: GPL-2.0-or-later

local ret=1

_arguments -C \
  '(* -)'{-h,--help}'[Displays help on commandline options]' \
  '--select[The files and folders passed as arguments will be selected.]' \
  '--split[Dolphin will get started with a split view.]' \
  '--new-window[Dolphin will explicitly open in a new window.]' \
  '--daemon[Start Dolphin Daemon (only required for DBus Interface).]' \
  '*:: :_urls' \
  && ret=0

return $ret
