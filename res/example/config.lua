----
--
--
config = {}
config.handler = {}
config.handler["^/$"]           = "index.lua"
config.handler["^/index.html$"] = "index.lua"
config.handler["^/json$"]       = "json.lua"
config.handler["^/redirect$"]   = "redirect.lua"
config.handler["^/database$"]   = "database.lua"
config.handler["^/websock$"]    = "websock.lua"

config.mime = {}
config.mime["ttf"] = "application/octet-stream"

config.redirect = {
    {server.port, "^/test2$"  , HTTP_302_FOUND, "http", server.port, "/test"},
    {server.port, "^/.+%.css$", 0}, -- Process as usual.
    {server.port, "^/.+%.js$" , 0}, -- Process as usual.
    {server.port, "^/.+%.jpg$", 0}, -- Process as usual.
    {server.port, "^/.+%.png$", 0}, -- Process as usual.
    {server.port, "^/.+%.ico$", 0}, -- Process as usual.
    {server.port, "^/.+$", HTTP_403_FORBIDDEN} -- All other are forbidden.
}

config.idCookieExpire = "1m"

