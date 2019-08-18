if #arg < 3 then
    print("Usage:")
    print("", arg[0] .. " <SRC_FILE> <DST_FILE> <VAR_NAME>")
    print()
    print("DST_FILE can be '-' for standard output")
    print()

    os.exit(1)
end

SRC_FILE = arg[1]
DST_FILE = arg[2]
VAR_NAME = arg[3]

inf  = io.open(SRC_FILE, "rb")
if DST_FILE == "-" then
else
    outf = io.open(DST_FILE, "wb")
    io.output(outf)
end

bytes = inf:read("*a")

io.write("/* */\n")
io.write("static const char " .. VAR_NAME .. "[] = {\n    ")

for i = 1, #bytes do
    io.write("0x" ..  string.format("%02X", bytes:byte(i)) ..  ", ")
    if (i % 8) == 0 then
        io.write("\n    ")
    end
end

io.write("\n};\n")

inf:close()
if outf then
    outf:close()
end

