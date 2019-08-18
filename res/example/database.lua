----
--
--
repeat
    if request.qtable.action == "get" then
        local db, code, msg = sqlite3.open("database.db")
        if not db then
            return util.errorResponse(HTTP_500_INTERNAL_SERVER_ERROR)
        end

        local rows = {}
        for row in db:nrows("SELECT * FROM test") do
            table.insert(rows, row)
        end    

        local ok, encode = pcall(json.encode, {
            result = "ok",
            data = rows,
        })
        if not ok then
            return util.errorResponse(HTTP_500_INTERNAL_SERVER_ERROR)
        else
            response.headers["content-type"] = "application/json; UTF-8"
            return response:send(HTTP_200_OK, encode);
        end
    elseif request.qtable.action == "put" then
        local db, code = sqlite3.open("database.db")
        if not db then
            return util.errorResponse(HTTP_500_INTERNAL_SERVER_ERROR)
        end

        local res = db:exec[[
            CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, content TEXT);
        ]]
        if res ~= sqlite3.OK then
            util.debugPrint(DLEVEL_NOISE, "Database create error:", db:errmsg());
            return util.errorResponse(HTTP_500_INTERNAL_SERVER_ERROR)
        end

        local ok, decode = pcall(json.decode, util.getInput())
        if not ok then
            return util.errorResponse(HTTP_500_INTERNAL_SERVER_ERROR)
        else
            value = decode.value
        end

        local res = db:exec("INSERT INTO test (content) VALUES ('" .. decode.value .."');")
        if res ~= sqlite3.OK then
            util.debugPrint(DLEVEL_NOISE, "Database insert error:", db:errmsg());
            return util.errorResponse(HTTP_500_INTERNAL_SERVER_ERROR)
        end

        local ok, encode = pcall(json.encode, {
            result = "ok",
        })
        if not ok then
            return util.errorResponse(HTTP_500_INTERNAL_SERVER_ERROR)
        else
            response.headers["content-type"] = "application/json; UTF-8"
            return response:send(HTTP_200_OK, encode);
        end
    else
        return response:send(HTTP_400_BAD_REQUEST)
    end
until true

