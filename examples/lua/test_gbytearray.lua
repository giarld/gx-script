---
--- Created by Gxin.
--- DateTime: 2022/5/21 13:13
---

--- GByteArray ---
print("\n --- GByteArray test ---\n");

local GByteArray = GAny._import("Gx.GByteArray");
local GFile = GAny._import("Gx.GFile");
local GCrypto = GAny._import("Gx.GCrypto");
local GHashSum = GAny._import("Gx.GHashSum");

local gba = GByteArray();

gba:writeInt8(1);
gba:writeInt16(2);
gba:writeInt32(3);
gba:writeInt64(4);
gba:writeUInt8(5);
gba:writeUInt16(6);
gba:writeUInt32(7);
gba:writeUInt64(8);
gba:writeFloat(9.0);
gba:writeDouble(10.0);
gba:writeString("hello");
-- 支持不规则Table的读写
local tTable = { 10, 2.1, s = "ctr", true, 0xfff, more = { 1, 2, 3 }, [{ 1, 2 }] = "table key", ["end"] = "end" };
tTable["more"]["end"] = 4;
gba:writeTable(tTable);

local item = GByteArray();
item:writeString("item");
-- 支持以GByteArray为元素的table读写
gba:writeTable({ a = 1, b = item });

print("gba: ", gba);

local gba2 = GByteArray();
gba2:writeBytes(gba);
print("gba2: ", gba2);

local eKey = GCrypto.signKeyPair();

local gbaEn = GCrypto.sign(gba2, eKey.secureKey);   -- encryption
print("gbaEn: ", gbaEn);

local gba3 = GCrypto.signOpen(gbaEn, eKey.publicKey):readBytes();   -- decryption
print("gba3: ", gba3);

local v1 = gba3:readInt8();
local v2 = gba3:readInt16();
local v3 = gba3:readInt32();
local v4 = gba3:readInt64();
local v5 = gba3:readUInt8();
local v6 = gba3:readUInt16();
local v7 = gba3:readUInt32();
local v8 = gba3:readUInt64();
local v9 = gba3:readFloat();
local v10 = gba3:readDouble();
local v11 = gba3:readString();
local v12 = gba3:readTable();
local v13 = gba3:readTable();

print("v1: ", v1);
print("v2: ", v2);
print("v3: ", v3);
print("v4: ", v4);
print("v5: ", v5);
print("v6: ", v6);
print("v7: ", v7);
print("v8: ", v8);
print("v9: ", v9);
print("v10: ", v10);
print("v11: ", v11);
print("v12: ", v12:_toString());
print("v13: ", string.format("{a=%d, b=%q}", v13.a, v13.b:_toString()));
print("v13.b type: ", v13.b:_classTypeName());
print("v13.b readString: ", v13.b:readString());

local gba4 = GByteArray(gba3);
local gb4base64 = GByteArray.base64Encode(gba4);
print("gb4base64: ", gb4base64);
local gba5 = GByteArray.base64Decode(gb4base64);
print("gba5: ", gba5);
local gba6 = GByteArray.compress(gba5);
print("gba6: ", gba6);
print("gb6base64: ", GByteArray.base64Encode(gba6));

local file4 = GFile("./b.dat");
if file4:open("wb+") then
    file4:write(gba);
    file4:close();
else
    print("file4 open failed");
end

local file5 = GFile("./b.dat");
if file5:open("rb") then
    local gba7 = file5:read();
    print("gba7: ", gba7);
    file5:close();

    -- 计算gba7(b.dat)的md5
    local hashJob = GHashSum.hashSum(GHashSum.Md5);
    hashJob:update(gba7);
    local hash = hashJob:final();
    print("gba7 md5: ", hash:toHexString());
else
    print("file5 open failed");
end

----- GUuid ---

local GUuid = GAny._import("Gx.GUuid");
local uuid = GUuid();
print("uuid: ", uuid);
print("uuid: ", uuid:toString(GUuid.B));
print("uuid: ", uuid:toString(GAny._import("Gx.GUuidFormatType").P));
