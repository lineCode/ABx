include("/scripts/includes/chat.lua")
include("/scripts/includes/consts.lua")

name = "Merchant"
level = 20
modelIndex = 9     -- Merchant body model
sex = SEX_MALE
creatureState = CREATURESTATE_IDLE
prof1Index = 1     -- Warrior
prof2Index = 0     -- None

local clickCount = 0
function onInit()
  return true
end

function onUpdate(timeElapsed)

end

function onClicked(creature)
  self:FaceObject(creature)
  clickCount = clickCount + 1
  if (clickCount > 3) then
    self:Say(CHAT_CHANNEL_GENERAL, "WTF! Go away!")
    self:SetState(CREATURESTATE_EMOTE_TAUNT)
    clickCount = 0
  end
end

-- self was selected by creature
function onSelected(creature)
  if (creature:IsDead()) then
    self:Say(CHAT_CHANNEL_GENERAL, "Wow, how did you manage to die here? Noob!")
  else
    self:Say(CHAT_CHANNEL_GENERAL, "What do you want?!?")
  end
--  print(creature:GetName() .. " selected me, the " .. self:GetName() .. " :D")
  -- Testing Raycast
--  local pos = creature:GetPosition();
--  print("Raycast to " .. pos[1] .. "," .. pos[2] .. "," .. pos[3])
--  local objects = self:Raycast(pos[1], pos[2], pos[3]);
--  for i, v in ipairs(objects) do
--    print(i, v, v:GetName()) 
--  end
end

-- creature collides with self
function onCollide(creature)
end
