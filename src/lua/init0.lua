----
--
--

-- fix RESOURCE_DIR variable
if RESOURCE_DIR:sub(#RESOURCE_DIR) == "/" then
    RESOURCE_DIR = RESOURCE_DIR:sub(1, #RESOURCE_DIR - 1)
end


Request  = {}
Request.__index = Request

Response = {}
Response.__index = Response

----
-- Send headers, contentLength field and optionally content.
--
function Response:send(errCode, content, contentLength)
    local r = {}
    local append = function(data)
        table.insert(r, data)
    end

    if not errCode or errCode == HTTP_403_FORBIDDEN then
        append("HTTP/1.1 403 Forbidden\r\n")
    elseif errCode == HTTP_101_SWITCHING_PROTOCOLS then
        append("HTTP/1.1 101 Switching Protocols\r\n")
    elseif errCode == HTTP_200_OK then
        append("HTTP/1.1 200 OK\r\n")
    elseif errCode == HTTP_301_MOVED_PERMANENTLY then
        append("HTTP/1.1 301 Moved Permanently\r\n")
    elseif errCode == HTTP_302_FOUND then
        append("HTTP/1.1 302 Found\r\n")
    elseif errCode == HTTP_303_SEE_OTHER then
        append("HTTP/1.1 303 See Other\r\n")
    elseif errCode == HTTP_304_NOT_MODIFIED then
        append("HTTP/1.1 304 Not Modified\r\n")
    elseif errCode == HTTP_307_TEMPORARY_REDIRECT then
        append("HTTP/1.1 307 Temporary Redirect\r\n")
    elseif errCode == HTTP_400_BAD_REQUEST then
        append("HTTP/1.1 400 Bad Request\r\n")
    elseif errCode == HTTP_404_NOT_FOUND then
        append("HTTP/1.1 404 Not Found\r\n")
	elseif errCode == HTTP_500_INTERNAL_SERVER_ERROR then
        append("HTTP/1.1 500 Internal Server Error\r\n")
    else
		errCode = HTTP_403_FORBIDDEN
        append("HTTP/1.1 403 Forbidden\r\n")
    end
    append("Server: Luno\r\n")

    if self.headers["content-type"] then
        append("content-type: " .. self.headers["content-type"])
    else
        append("content-type: text/html; charset=utf-8")
    end
    append("\r\n")

    if not contentLength then
        if content then
            contentLength = #content
        else
            contentLength = 0
        end
    end
    if contentLength then
        append("content-length: " .. contentLength .. "\r\n")
    end

--    if self.headers and not self.headers["cache-control"] then
--        if server.caching() then
--            self.headers["cache-control"] = "private, max-age=60"
--        else
--            self.headers["cache-control"] = "no-store"
--        end
--    end

    for k, v in pairs(self.headers) do
        if k ~= "connection" and
           k ~= "content-type"
        then
            if type(v) == "table" then
                for tk, tv in pairs(v) do
                    append(k .. ": " .. tv .. "\r\n")
                end
            else
                append(k .. ": " .. v .. "\r\n")
            end
        end
    end

    --
    -- Connection header.
    --
    append("connection: ")
    if self.headers["connection"] ~= nil then
        append(self.headers["connection"])
        append(", ")
    end
    if request.keepAlive then
        append("keep-alive\r\n")
    else
        append("close\r\n")
    end

    append("\r\n")

    self.writeSock(table.concat(r))

    if content then
        self.writeSock(content)
    end

	return true
end
----
--
--
function Response:redirect(path)
    local redirect = {}

    if self.secure then
        table.insert(redirect, "https://")
    else
        table.insert(redirect, "http://")
    end
    table.insert(redirect, self.host)
    table.insert(redirect, path)
    if self.query then
        table.insert(redirect, self.query)
    end

    self.headers["location"] = table.concat(redirect)
    return self:send(HTTP_302_FOUND)
end
----
-- Execute "config.lua" file.
--
if true then
    local configFile = "config.lua"

    config = {}
    if util.fileExists(configFile, false) then
        local ok, msg = util.doFile(configFile, false)
        if not ok then
            util.debugPrint(DLEVEL_ERROR, "(E) Failed to execute " .. configFile .. ": " .. msg)
        end
    end
end

