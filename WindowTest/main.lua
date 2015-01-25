CreateKeyBoardStick(100,500,200,"WSAD")
CreateMouseStick(800,450,250,40,0.4,0.7)
CreateButton(1060,450,100,250,string.byte(' '))
local up=CreateButton(800,710,100,80,string.byte('-'))
local down=CreateButton(910,710,100,80,string.byte('-'))
local rck=CreateButton(400,500,100,100,0,1)

local index=49
 
function bdec()
MouseWheel(-120)
end

function badd()
MouseWheel(120)
end

function sw()
KeyPress("3")
end

RegisterButtonEvent(up,bdec,ButtonUp)
RegisterButtonEvent(down,badd,ButtonUp)
RegisterButtonEvent(rck,sw,ButtonUp)