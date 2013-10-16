--[[ LOCAL VARIABLES AND ARGUMENTS --]]
local function foo1(arg1, arg2)
	local a = 0;
	print(a);
	do
		local a = 1;
		print(a);
	end
	if a == 0 then
		local a = 2;
		print(a);
	end
	print("args: ", arg1, arg2);
end

foo1(1, 2);