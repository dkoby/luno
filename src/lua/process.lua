----
--
--
function process()
    --
    -- Convert request headers names to lower case.
    --
    if true then
        local headers = {}
        for k, v in pairs(request.headers) do
            headers[tostring(k):lower()] = v
        end
        request.headers = headers
    end
    
    if httpError ~= HTTP_200_OK then
        return util.errorResponse(httpError)
    end

	util.debugPrint(DLEVEL_NOISE, "REQUEST PATH:", request.path)
    --
    -- config.lua not allowed.
    --
    if request.path:match("config.lua$") then
        return util.errorResponse(HTTP_404_NOT_FOUND)
    end
    --
    -- Process session.
    --
    repeat
        local SESCOOKIE = "LSESSIONID"
        local id
        if request.headers["cookie"] then
            id = string.match(request.headers["cookie"], SESCOOKIE .. "=([^;]+)")
        end
        if id then
            if not server.hasSession(id) then
                util.debugPrint(DLEVEL_NOISE, "Server has not session:", id)
                id = nil
            end
        end
        if not id then
            id = server.newSession()
            local expire = 0 -- seconds
            if config.idCookieExpire then
                repeat
                    local value
                    value = config.idCookieExpire:match("(%d+)m")
                    if value then
                        expire = 60 * value
                        break
                    end
                    value = config.idCookieExpire:match("(%d+)h")
                    if value then
                        expire = 60 * 60 * value
                        break
                    end
                    value = config.idCookieExpire:match("(%d+)d")
                    if value then
                        expire = 24 * 60 * 60 * value
                        break
                    end
                until true
            end
            expire = math.floor(expire)
            local expireString
            if expire > 0 then
                expireString = os.date("!%a, %d %b %Y %H:%M:%S GMT", os.time() + expire)
            end
            local setCookie = {}
            table.insert(setCookie, SESCOOKIE)
            table.insert(setCookie, "=")
            table.insert(setCookie, id)
            if expireString then
                table.insert(setCookie, "; ")
                table.insert(setCookie, "Expires=")
                table.insert(setCookie, expireString)
            end
            table.insert(setCookie, ";")
            table.insert(response.headers["set-cookie"], table.concat(setCookie))
        end
        request.sessionId = id
    until true
    --
    -- First lookup in "handler" table for appropriate processor.
    --
    if config.handler then
        for pattern, path in pairs(config.handler) do
            if string.match(request.path, pattern) then
                local ok, msg = util.doFile(path, false)
                if not ok then
					util.debugPrint(DLEVEL_ERROR, "Failed to run", path, ":", msg)
					return util.errorResponse(HTTP_500_INTERNAL_SERVER_ERROR)
                end
				return ok
            end
        end
    end
    --
    -- Lookup redirection table.
    --
    if config.redirect then
        for i, entry in ipairs(config.redirect) do
            local fromPort        = entry[1]
            local fromPathPattern = entry[2]
            local errCode         = entry[3]
            local proto           = entry[4]
            local toPort          = entry[5]
            local toPath          = entry[6]

            if fromPort ~= -1 and toPort ~= -1 then
                if request.serverPort == fromPort then
                    if string.match(request.path, fromPathPattern) then
						if errCode == 0 then
							break
						end

                        if errCode == HTTP_403_FORBIDDEN then
                            return util.errorResponse(HTTP_403_FORBIDDEN)
                        end

                        local host, port = string.match(request.headers["host"], "([^:]*):?(%d*)$")

                        local redirect = {}
                        table.insert(redirect, proto)
                        table.insert(redirect, "://")
                        table.insert(redirect, host)
                        if port ~= "" then
                            table.insert(redirect, ":")
                            table.insert(redirect, toPort)
                        end
                        table.insert(redirect, toPath)
                        if request.query then
                            table.insert(redirect, request.query)
                        end
                        redirect = table.concat(redirect)

                        util.debugPrint(DLEVEL_NOISE, "REDIRECT ", redirect)

                        response.headers["location"] = redirect
                        return response:send(errCode)
                    end
                end
            end
        end
    end
    --
    -- Lookup for common files.
    --
    if true then
		local path

        path = string.match(request.path, "/$")
        if path then
            return util.outputFile("index.html", "text/html; charset=utf-8", false)
        end
        if config.mime then
            local ext = string.match(request.path:lower(), "%.([%w_]*)$")
            if config.mime[ext] then
                return util.outputFile(string.match(request.path, "/(.*)$"), config.mime[ext], false)
            end
        else
            util.debugPrint(DLEVEL_NOISE, "NO MIME CONFIG")
        end
        path = string.match(request.path, "/(.*%.html)$")
        if path then
            return util.outputFile(path, "text/html; charset=utf-8", false)
        end
        path = string.match(request.path, "/(.*%.css)$")
        if path then
            return util.outputFile(path, "text/css; charset=utf-8", false)
        end
        path = string.match(request.path, "/(.*%.js)$")
        if path then
            return util.outputFile(path, "application/x-javascript; charset=utf-8", false)
        end
        path = string.match(request.path, "/(.*%.ico)$")
        if path then
            return util.outputFile(path, "image/vnd.microsoft.icon", false)
        end
        path = string.match(request.path, "/(.*%.png)$")
        if path then
            return util.outputFile(path, "image/png", false)
        end
        path = string.match(request.path, "/(.*%.svg)$")
        if path then
            return util.outputFile(path, "image/svg+xml", false)
        end
    end
    --
    -- Not found.
    --
    return util.errorResponse(HTTP_404_NOT_FOUND)
end

process()

