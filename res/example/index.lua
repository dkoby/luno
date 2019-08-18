----
--
--
repeat
    --
    -- Print headers to console.
    --
    for k, v in pairs(request.headers) do
        util.debugPrint(DLEVEL_NOISE, "HEADER:", k, v)
    end
    --
    -- Print query parameters to console.
    --
    for k, v in pairs(request.qtable) do
        util.debugPrint(DLEVEL_NOISE, "PARAM:", k, v)
    end

    local user = server.getSessionString("user")
    if not user then
        -- Session expired.
        server.setSessionString("user", "Test user");
        user = "unknown";
    end

    local sb = util.StringBuilder:new()

    sb:appendn('<!DOCTYPE html>')
    sb:appendn('<html>')
    sb:appendn('<head>')
    sb:appendn('<meta charset="UTF-8"/>')
    sb:appendn('<title>Luno</title>')
    sb:appendn('<link rel="stylesheet" href="css/style.css">')
    sb:appendn('<link rel="icon" href="favicon.ico">')
    sb:appendn('</head>')
    sb:appendn('<body>')
    sb:appendn('<h1>Hello from luno.</h1>')
    sb:appendn('<a href="redirect">Test redirection</a>')
    sb:appendn('<p>User is: ' .. user .. '</p>')
    sb:appendn('<script src="js/script.js"></script>')
    sb:appendn('</body>')
    sb:appendn('</html>')

    table.insert(response.headers["set-cookie"], "My cookie = 12345;")

    return response:send(HTTP_200_OK, sb:get())
until true

