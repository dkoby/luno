----
--
--
request = {}
setmetatable(request, Request)

response = {}
response.headers = {}
response.headers["set-cookie"] = {}
setmetatable(response, Response)

