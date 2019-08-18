----
--
--
repeat
    local value = 0

    if request.method ~= "POST" then
        return util.errorResponse(HTTP_400_BAD_REQUEST)
    end

    local ok, decode = pcall(json.decode, util.getInput())
    if not ok then
        return util.errorResponse(HTTP_500_INTERNAL_SERVER_ERROR)
    else
        value = decode.value
    end

    local ok, encode = pcall(json.encode, {
        ["result"] = "ok",
        ["data"] = {
            ["value"] = value + 1, 
        }
    })
    if not ok then
        return util.errorResponse(HTTP_500_INTERNAL_SERVER_ERROR)
    else
        response.headers["content-type"] = "application/json; UTF-8"
        return response:send(HTTP_200_OK, encode);
    end
until true

