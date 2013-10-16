print("Hello World!");

--[[ REMOVE COMMENT ]]
-- comment
--[[ comment ]]

--[[ REMOVE DUPLICATE CHARS ]]
--[[ REMOVE EXTRA LINE LINE ]]
i      =    1   ;
j   =     i      +         2;
k =   j *    2;

--[[ REMOVE EXTRA WHITESPACE ]]
t = {}; t = { };
i = ( 1 ) + 2; i = ( 1 ) + ( 2 );
t[1] = 1; t [ 1 ] = 1;
t.name = "name"; t. name = "name";
local a, b = 1, 2;
i = 1 + 2 - 3 / 4 * 5 ^ 6 % 7;
if i == 0 then
	i = 0;
end
if i ~= 0 then
	i = 0;
end
i = #t;

--[[ GLOBAL FUNCTION NAMES ]]

function f1(a)
	print(a);
end

function f2(a, b)
	print(a, b);
end

f1(1);
f2(2, 2);

--[[ LOCAL VARIABLES AND ARGUMENTS]]
function foo(arg1, arg2, arg3)
	print(arg1, arg2, arg3);
	arg2 = arg1;
	arg3 = arg1;
	print(arg1, arg2, arg3);
end

foo(1, 2, 3);

--[[ 2 ]]
function foo(arg1, arg2)
	print(arg1, arg2);
	print("a: ", a);

	do
		local a = 2;
		print("local a: ", a);
	end

	for a = 1, 5 do
		local a = 3;
		print("a in loop FOR: ", a);
	end

	i = 0;
	while i < 3 do
		local a = 44;
		print("a in loop WHILE", a);
		i = i + 1;
	end

	if true then
		local a = 5;
		print("a in IF: ", a);
	end

	f1 = function(a1, a2)
		print(a1, a2);
		local a = 6;
		print("a in function", a);
	end
	
	f1(111, 222);
end
	
foo("arg1", "arg2");

