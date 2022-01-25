local core = require "core"
local command = require "core.command"
local keymap = require "core.keymap"
local console = require "plugins.console"

command.add(nil, {
  ["project:build-project"] = function()
    core.log "Building..."
    console.run {
      command = "g++ breakout.cpp -o breakout -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++20 && ./breakout",
      file_pattern = "(.*):(%d+):(%d+): (.*)$",
      cwd = ".",
      on_complete = function(retcode) core.log("Build complete with return code "..retcode) end,
    }
  end
})

keymap.add { ["ctrl+b"] = "project:build-project" }

