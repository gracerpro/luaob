-- test local vars and params
local a = 999;

if a = 2 then
	print(a);
	local a = 2;
	print(a);
	if a = 2 then
		print(a);
		local a = 3;
		print(a);
	end
end
