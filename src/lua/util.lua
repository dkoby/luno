----
--
--
util = {}
----
--
--
function util.debugPrint(level, ...)
    if level ~= DLEVEL_SYS and level > server.debugLevel then
        return
    end

    local s = {}
    for i, v in ipairs({...}) do
        table.insert(s, v)
    end
    server.debugPrint(level, table.concat(s, " "))
end
----
--
--
function util.fileExists(path, external)
    if external == nil then
        external = true
    end
    if not external then
        path = RESOURCE_DIR .. "/" .. path
    end

    if path:match(MFS_PREFIX) then
        local fd = mromfs.open(path:sub(#MFS_PREFIX + 1))
        if not fd then
            return false
        else
            return true
        end
    end

    local function tryToOpen(path)
        fd = io.open(path, "r")
        fd:close()
    end
    local ok, msg = pcall(tryToOpen, path)
    if not ok then
        return false
    else
        return true
    end
end
----
--
--
function util.doFile(path, external)
    local chunk

    if external == nil then
        external = true
    end
    if not util.fileExists(path, external) then
        return util.errorResponse(HTTP_404_NOT_FOUND)
    end
    if not external then
        path = RESOURCE_DIR .. "/" .. path
    end

    util.debugPrint(DLEVEL_NOISE, "DO FILE: ", path)

    if path:match(MFS_PREFIX) then
        local fd = mromfs.open(path:sub(#MFS_PREFIX + 1))
        if not fd then
            return false, "Failed to open mromfs file: " .. path
        end

        local loadFile = function()
            local CHUNK_SIZE = 256
            return mromfs.read(fd, CHUNK_SIZE)
        end

        local msg
        chunk, msg = load(loadFile)
        if not chunk then
            return false, msg
        end
    else
        local msg
        chunk, msg = loadfile(path)
        if not chunk then
            return false, msg
        end
    end

    return pcall(chunk)
end
----
--
--
function util.outputFile(path, contentType, external)
    if external == nil then
        external = true
    end

    if not util.fileExists(path, external) then
        return util.errorResponse(HTTP_404_NOT_FOUND)
    end
    if not external then
        path = RESOURCE_DIR .. "/" .. path
    end

    util.debugPrint(DLEVEL_NOISE, "OUTPUT FILE:", path)

    if not contentType then
        contentType = "text/plain"
    end
    response.headers["content-type"] = contentType;

    local data
    if path:match(MFS_PREFIX) then
        local CHUNK_SIZE = 256
        local fd = mromfs.open(path:sub(#MFS_PREFIX + 1))

        data = {}
        while true do
            local chunk = mromfs.read(fd, CHUNK_SIZE)
            if not chunk or #chunk == 0 then
                break
            end
            table.insert(data, chunk)
        end
        data = table.concat(data)
    else
        local fd = io.open(path, "rb")
        data = fd:read("a")
        fd:close()
    end

    response:send(HTTP_200_OK, data)
end
----
--
--
function util.errorResponse(errCode, short)
    local errFile = "error/error" .. errCode .. ".lua"

    if short or not util.fileExists(errFile, false) then
        return response:send(errCode)
    else
        local ok, msg = util.doFile(errFile, false)
        if not ok then
            util.debugPrint(DLEVEL_ERROR, "Failed to execute error file: ", msg)
        end
		return ok, msk
    end
end
----
--
--
function util.getInput()
    local input = {}
    if request.contentLength then
        local n = request.contentLength
        while n > 0 do
            local chunkSize = 256

            if chunkSize > n then
                chunkSize = n
            end

            local chunk = request.getContent(chunkSize)
            if #chunk == 0 then
                break
            end
            n = n - #chunk
            table.insert(input, chunk);
        end
    end
    return table.concat(input);
end
-------------------------------------------------------------------------------
-- String buffer for ease of output.
-------------------------------------------------------------------------------
util.StringBuilder = {}
function util.StringBuilder:new()
    local obj = {}

    setmetatable(obj, self)
    self.__index = self

    obj.val = {}
    return obj
end
function util.StringBuilder:append(...)
    for i, v in ipairs({...}) do
        table.insert(self.val, v)
    end
end
function util.StringBuilder:appendn(...)
    self:append(...)
    table.insert(self.val, v)
end
function util.StringBuilder:get()
    return table.concat(self.val)
end


