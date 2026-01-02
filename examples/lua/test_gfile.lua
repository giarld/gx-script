---
--- Created by Gxin.
--- DateTime: 2022/5/21 13:12
---

--- GFile ---
print("\n --- GFile test ---\n");

local GFile = GAny._import("Gx.GFile");

local file1 = GFile("./");
print("file1:", file1);

if file1:exists() then
    print("file1 exists");
    if file1:isFile() then
        print("file1 is file");
    else
        print("file1 is dir");
        local files = file1:listFiles(function(f)
            return f:isFile();
        end);
        print("files: ", #files);
        for i, f in ipairs(files) do
            print(string.format("  %d. %q", i, f:fileName()));
        end
    end
else
    print("file1 not exists");
end

local file2 = GFile("./");
file2 = file2 / "a.txt"; -- 可以这样连接文件
print("file2:", file2:absoluteFilePath());
if file2:open(GFile.ReadWrite) then  -- 枚举同: GFile.OpenMode.ReadWrite
    print("file2 open success");
    file2:write("hello world, 你好世界");
    file2:close();
else
    print("file2 open failed");
end

local file3 = GFile("./a.txt");
if file3:open("r") then
    print("file3 open success");
    print("file3 content: ", file3:readAll());
    file3:close();
else
    print("file3 open failed");
end

local file4 = GFile("./ab.txt");
if file4:exists() then
    file4:remove();
end

if file3:rename(file4) then
    print("file3 -> file4");
end

collectgarbage();
