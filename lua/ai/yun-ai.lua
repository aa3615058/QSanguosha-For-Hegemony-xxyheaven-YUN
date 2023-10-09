--怀贝贝
--天成
sgs.ai_skill_invoke.tiancheng = function(self, data)
	if not (self:willShowForAttack() or self:willShowForDefence()) then return false end
	return true
end

function sgs.ai_cardneed.tiancheng(to, card)
	return card:getSuit() == sgs.Card_Spade
end

--韩静
--连击
--巧破

--王灿
--丝舞
sgs.ai_skill_invoke.siwu = function(self, data)
	if not self:willShowForDefence() then return false end
	return true
end

sgs.ai_skill_choice["siwu_draw"] = function(self, choices, data)
	if not (self:willShowForAttack() or self:willShowForDefence()) then return "no" end
	if self:needKongcheng() then return "no" end
	return "yes"
end

--星灿

--杨文琦
--战鬼

--调略
sgs.ai_skill_choice["diaolue_draw"] = function(self, choices, data)
	if not (self:willShowForAttack() or self:willShowForDefence()) then return "no" end
	return "yes"
end

--肖洒
--潇寒一效果
sgs.ai_skill_invoke.xiaohan = function(self, data)
	if not (self:willShowForAttack() or self:willShowForDefence()) then return false end
	return true
end

--潇寒二效果
sgs.ai_skill_choice["@xiaohan-ice-sword"] = function(self, choices, data)
	if sgs.ai_skill_invoke.IceSword(self,data) then
		return "yes"
	else return "no"
	end
end

--秘雨

--李淑玉
--影咒
--奇风

--李云鹏

--炫阳
sgs.ai_skill_invoke.xuanyang = sgs.ai_skill_invoke.DoubleSword
sgs.ai_skill_cardask["@xuanyang-discard"] = sgs.ai_skill_cardask["double-sword-card"]

--慧黠
sgs.ai_skill_invoke.huixia = function(self, data)
	if not (self:willShowForAttack() or self:willShowForDefence()) then return false end
	return true
end

--闭月
sgs.ai_skill_invoke.biyue_liyunpeng = sgs.ai_skill_invoke.biyue

--良才
sgs.ai_skill_invoke.liangcai = function(self, data)
	if self:needRetrial(data:toJudge()) then
		return true
	end
	return false
end

--病酒
