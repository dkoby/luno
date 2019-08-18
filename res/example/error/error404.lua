----
--
--
repeat
    local sb = util.StringBuilder:new()

    sb:appendn('<!DOCTYPE html>')
    sb:appendn('<html>')
    sb:appendn('<head>')
    sb:appendn('<meta charset="UTF-8"/>')
    sb:appendn('<title>Luno, 404</title>')
    sb:appendn('<link rel="stylesheet" href="css/style.css">')
    sb:appendn('</head>')
    sb:appendn('<body class="error">')
    sb:appendn('<h1>404 Not found.</h1>')
    sb:appendn('</body>')
    sb:appendn('</html>')

    return response:send(HTTP_404_NOT_FOUND, sb:get())
until true
