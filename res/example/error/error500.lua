----
--
--
repeat
    local sb = util.StringBuilder:new()

    sb:appendn('<!DOCTYPE html>')
    sb:appendn('<html>')
    sb:appendn('<head>')
    sb:appendn('<meta charset="UTF-8"/>')
    sb:appendn('<title>Luno, 500</title>')
    sb:appendn('<link rel="stylesheet" href="css/style.css">')
    sb:appendn('</head>')
    sb:appendn('<body class="error">')
    sb:appendn('<h1>500 Internal Server Error.</h1>')
    sb:appendn('</body>')
    sb:appendn('</html>')

    return response:send(HTTP_500_INTERNAL_SERVER_ERROR, sb:get())
until true
