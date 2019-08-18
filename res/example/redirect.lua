----
--
--
repeat
    response:redirect(request.headers["host"] .. "/")
until true

