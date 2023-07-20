sgs.ai_skill_invoke.tiancheng = function(self, data)
	if not (self:willShowForAttack() or self:willShowForDefence()) then return false end
	return true
end

function sgs.ai_cardneed.tiancheng(to, card)
	return card:getSuit() == sgs.Card_Spade
end

--qiaopo

--siwu
sgs.ai_skill_invoke.siwu = function(self, data)
	if not self:willShowForDefence() then return false end
	return true
end

--siwu-draw
sgs.ai_skill_choice["siwu_draw"] = function(self, choices, data)
	if not (self:willShowForAttack() or self:willShowForDefence()) then return "no" end
	return "yes"
end

--xingcan

--zhangui

--diaolue
sgs.ai_skill_choice["diaolue_draw"] = function(self, choices, data)
	if not (self:willShowForAttack() or self:willShowForDefence()) then return "no" end
	return "yes"
end

--xiaohan
sgs.ai_skill_invoke.xiaohan = function(self, data)
	if not (self:willShowForAttack() or self:willShowForDefence()) then return false end
	return true
end

--xiaohan_ice_sword

--miyu


--yingzhou

--qifeng


