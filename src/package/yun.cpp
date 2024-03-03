#include "settings.h"
#include "standard-basics.h"
#include "skill.h"
#include "yun.h"
#include "standard-qun-generals.h"
#include "strategic-advantage.h"
#include "client.h"
#include "engine.h"
#include "ai.h"
#include "general.h"
#include "clientplayer.h"
#include "clientstruct.h"
#include "room.h"
#include "roomthread.h"
#include "json.h"
#include "protocol.h"

class Zhuyan : public TriggerSkill {
public:
    Zhuyan() : TriggerSkill("zhuyan") {
        events << GeneralShown << GeneralHidden << EventLoseSkill << DFDebut;
        frequency = Compulsory;
    }

    virtual bool canPreshow() const {
        return false;
    }

    virtual void record(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const {
        if (player == NULL) return;
        bool has_head_hongyan = false;
        bool has_deputy_hongyan = false;
        bool has_deputy_zhuyan = false;

        foreach (const Skill *skill, player->getHeadSkillList(true, true)) {
            if (skill->objectName() == "hongyan") {
                has_head_hongyan = true;
            }
        }

        foreach (const Skill *skill, player->getDeputySkillList(true, true)) {
            if (skill->objectName() == "hongyan") {
                has_deputy_hongyan = true;
            }else if (skill->objectName() == "zhuyan") {
                has_deputy_zhuyan = true;
            }
        }

        QString str_handle_skill = "hongyan_huaibeibei";
        if (player->hasShownSkill(objectName()) && !(player->hasShownAllGenerals() && (has_head_hongyan || has_deputy_hongyan))) {
            if(has_deputy_zhuyan) str_handle_skill += "!";
            room->handleAcquireDetachSkills(player, str_handle_skill);
        }
        else if (player->getAcquiredSkills().contains(str_handle_skill)) {
            str_handle_skill = "-" + str_handle_skill;
            room->handleAcquireDetachSkills(player, str_handle_skill);
        }

        return;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer* &) const {
        return QStringList();
    }
};

class HongyanHuaibeibeiFilter : public FilterSkill {
public:
    HongyanHuaibeibeiFilter() : FilterSkill("hongyan_huaibeibei") {}

    static WrappedCard *changeToHeart(int cardId) {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("hongyan_huaibeibei");
        new_card->setSuit(Card::Heart);
        new_card->setModified(true);
        return new_card;
    }

    virtual bool viewFilter(const Card *to_select, ServerPlayer *player) const {
        Room *room = player->getRoom();
        int id = to_select->getEffectiveId();
        if (player->hasShownSkill("hongyan_huaibeibei"))
            return to_select->getSuit() == Card::Spade
                    && (room->getCardPlace(id) == Player::PlaceEquip
                        || room->getCardPlace(id) == Player::PlaceHand || room->getCardPlace(id) == Player::PlaceJudge);

        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        return changeToHeart(originalCard->getEffectiveId());
    }
};

class HongyanHuaibeibei : public TriggerSkill {
public:
    HongyanHuaibeibei() : TriggerSkill("hongyan_huaibeibei") {
        frequency = Compulsory;
        view_as_skill = new HongyanHuaibeibeiFilter;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer * &) const {
        return QStringList();
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const {
        return -2;
    }
};

class HongyanHuaibeibeiMaxCards : public MaxCardsSkill {
public:
    HongyanHuaibeibeiMaxCards() : MaxCardsSkill("#hongyan_huaibeibei-maxcards") {}

    virtual int getExtra(const Player *target) const {
        if (target->hasShownSkill("hongyan_huaibeibei")) {
            foreach (const Card *equip, target->getEquips()) {
                if (equip->getSuit() == Card::Heart)
                    return 1;
            }
        }
        return 0;
    }
};

class Tiancheng : public TriggerSkill {
public:
    static QString mark_tiancheng_times;

     Tiancheng() : TriggerSkill("tiancheng") {
         events << CardResponded << CardUsed << FinishJudge;
         frequency = Frequent;
     }

     virtual QStringList triggerable(TriggerEvent event, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
     {
         if (!TriggerSkill::triggerable(player))
             return QStringList();

         const Card *card = NULL;
         if (event == CardResponded) {
             card = data.value<CardResponseStruct>().m_card;
         } else if(event == CardUsed) {
             card = data.value<CardUseStruct>().card;
         } else if(event == FinishJudge) {
             card = data.value<JudgeStruct *>()->card;
         }

         int count = 0;
         if(card) {
             if(card->getSkillName().contains("hongyan")) {
                 count++;
             }else if (!card->isVirtualCard() && card->subcardsLength() > 0)  {
                 QList<int> subcards = card->getSubcards();
                 foreach (int cardID, subcards) {
                     if(Sanguosha->getCard(cardID)->getSkillName().contains("hongyan")) {
                         count++;
                         break;
                     }
                 }
             }
         }

         if(count > 0) return QStringList(objectName());
         else return QStringList();
     }

     virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
         if (player->askForSkillInvoke(this, data)) {
             room->broadcastSkillInvoke(objectName(), player);
             return true;
         }
         return false;
     }

     virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
         player->drawCards(1, objectName());
         int times = player->getMark(mark_tiancheng_times);
         if(player->getPhase() == Player::Play) {
             if(player->getMark(mark_tiancheng_times)) {
                 room->askForDiscard(player, objectName(), 1, 1, false, true);

                 QString mark_tip = player->tag["tiancheng_mark_tip"].value<QString>();
                 if(mark_tip != NULL && !mark_tip.isNull() && !mark_tip.isEmpty())
                    room->setPlayerMark(player, mark_tip, 0);
                 mark_tip = QString("##tiancheng_tip+")+QString::number(times)+"-Clear";
                 room->setPlayerMark(player, mark_tip, 1);
                 player->tag["tiancheng_mark_tip"] = QVariant::fromValue(mark_tip);
             }
             room->setPlayerMark(player, mark_tiancheng_times, times+1);
         }

         return false;
     }
};

QString Tiancheng::mark_tiancheng_times = "tiancheng-Clear";

class Tianchengkeep : public MaxCardsSkill {
public:
    Tianchengkeep() : MaxCardsSkill("#tianchengkeep") {
        frequency = Compulsory;
    }

    int getExtra(const Player *target) const {
        if (target->hasShownSkill("tiancheng")) {
            int keep = target->getMark(Tiancheng::mark_tiancheng_times)-1;
            if(keep < 0) keep = 0;
            return keep;
        } else return 0;
    }
};


class Lianji : public TriggerSkill {
public:
     Lianji() : TriggerSkill("lianji") {
         events << Damage;
     }

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const {
         TriggerList skill_list;
         if (player == NULL) return skill_list;

         DamageStruct damage = data.value<DamageStruct>();
         if (damage.from == NULL || damage.to->isDead())
             return skill_list;

         QList<ServerPlayer *> jingmeizis = room->findPlayersBySkillName(objectName());
         foreach(ServerPlayer *jingmneizi, jingmeizis)
             if (jingmneizi->getPhase() == Player::NotActive && jingmneizi->canSlash(damage.to, NULL, true))
                 skill_list.insert(jingmneizi, QStringList(objectName()));

         return skill_list;
     }

     virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *jingmneizi) const {
         ServerPlayer *victim = data.value<DamageStruct>().to;
         if (room->askForUseSlashTo(jingmneizi, victim, QString("@lianji-prompt:%1").arg(victim->objectName()))) {
             room->broadcastSkillInvoke(objectName(), jingmneizi);
             return true;
         }
         return false;
     }

     virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *jingmneizi) const {
         jingmneizi->drawCards(1, objectName());
         return false;
     }
};

QiaopoCard::QiaopoCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}
void QiaopoCard::onEffect(const CardEffectStruct &effect) const {
    ServerPlayer *target = effect.to;
    DamageStruct damage = effect.from->tag.value("QiaopoDamage").value<DamageStruct>();
    target->obtainCard(this);
    DamageStruct damage2("qiaopo", damage.from, target, 1, damage.nature);
    damage2.transfer=true;
    damage2.transfer_reason="qiaopo";
    target->getRoom()->damage(damage2);
}
class QiaopoViewAsSkill : public OneCardViewAsSkill {
public:
    QiaopoViewAsSkill() : OneCardViewAsSkill("qiaopo") {
        response_pattern = "@@qiaopo";
        filter_pattern = ".|diamond";
    }
    const Card *viewAs(const Card *originalCard) const {
        QiaopoCard *qiaopoCard = new QiaopoCard;
        qiaopoCard->addSubcard(originalCard);
        return qiaopoCard;
    }
};
class Qiaopo : public TriggerSkill {
public:
    Qiaopo() : TriggerSkill("qiaopo") {
        events << DamageInflicted;
        view_as_skill = new QiaopoViewAsSkill;
    }
    virtual int getPriority() const {
        return -2;
    }

    virtual bool canPreshow() const {
        return true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (TriggerSkill::triggerable(player) && !player->isNude()) {
            return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        player->tag["QiaopoDamage"] = data;
        int qiaopoCount = 0;
        for(int i = 0; i < data.value<DamageStruct>().damage; i++) {
            if(room->askForUseCard(player, "@@qiaopo", "@qiaopo-card"))
                qiaopoCount++;
            else break;
        }
        player->tag["QiaopoCount"] = QVariant::fromValue(qiaopoCount);
        return qiaopoCount > 0;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        int qiaopoCount = player->tag["QiaopoCount"].value<int>();
        DamageStruct damage = data.value<DamageStruct>();
        if(qiaopoCount >= damage.damage) return true;
        else damage.damage -= qiaopoCount;
        data = QVariant::fromValue(damage);
        return false;
    }
};

class Siwu : public MasochismSkill {
public:
    Siwu() : MasochismSkill("siwu") {
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (TriggerSkill::triggerable(player))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName(), player);
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &) const {
        player->drawCards(1, objectName());
        if (!(player->isKongcheng())) {
            int cardID = player->getRoom()->askForExchange(player, objectName(), 1, 1, "@siwu-prompt", "", ".|.|.|hand").first();
            player->addToPile("&wire", cardID, false);
        }
    }
};

class SiwuDraw : public TriggerSkill {
public:
    SiwuDraw() : TriggerSkill("#siwu-draw") {
        events << CardUsed << CardResponded;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent event, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const {
        if (!TriggerSkill::triggerable(player))
            return QStringList();

        bool flag = false;
        if(event == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            flag = !use.m_isHandcard && !use.card->isKindOf("SkillCard");
        }else if(event == CardResponded) {
            CardResponseStruct res = data.value<CardResponseStruct>();
            flag = !res.m_isHandcard && !res.m_card->isKindOf("SkillCard");
        }

        if(flag) return QStringList(objectName());
        else return QStringList();
    }

    virtual bool cost(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        bool choice = room->askForChoice(player, "siwu_draw", "yes+no", QVariant(), "@siwu-draw") == "yes";

        if (choice) {
            LogMessage log;
            log.type = "#SiwuDraw";
            log.from = player;
            log.arg = event == CardUsed ?
                        data.value<CardUseStruct>().card->getEffectName():
                        data.value<CardResponseStruct>().m_card->getEffectName();
            log.arg2 = "siwu";
            room->sendLog(log);

            player->showSkill("siwu");
            room->broadcastSkillInvoke("siwu", player);

            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->drawCards(1, objectName());
        return false;
    }
};

class Xingcan : public TriggerSkill {
public:
    Xingcan() : TriggerSkill("xingcan") {
        events << EventPhaseEnd;
        frequency = NotFrequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer * &) const {
        if (!TriggerSkill::triggerable(player))
            return QStringList();

        if(player->getPhase() == Player::Play && !player->isSkipped(Player::Discard)) {
            return QStringList(objectName());
        }else
            return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        const Card* card = room->askForCard(player, ".", "@xingcan-put", QVariant(), Card::MethodNone);
        if(card) {
            room->broadcastSkillInvoke(objectName(), player);
            CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
            CardsMoveStruct move(card->getEffectiveId(), player, NULL, Player::PlaceHand, Player::DrawPile, reason);
            room->moveCardsAtomic(move, false);

            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->skip(Player::Discard);
        return false;
    }
};

class Zhangui : public TargetModSkill {
public:
    Zhangui() : TargetModSkill("zhangui") {}
    virtual bool canPreshow() const {
        return false;
    }

    virtual int getDistanceLimit(const Player *from, const Card *, const Player *) const {
        if (from->hasShownSkill(objectName()) && from->getPhase()==Player::Play)
            return 1024;
        else return 0;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const {
        if (from->hasShownSkill(objectName()) && from->getPhase()==Player::Play && from->getSlashCount() == 0)
            return 2;
        else
            return 0;
    }
};

class DiaolueVS : public OneCardViewAsSkill {
public:
    DiaolueVS() : OneCardViewAsSkill("diaolue") {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const {
        LureTiger *lureTiger = new LureTiger(originalCard->getSuit(), originalCard->getNumber());
        lureTiger->addSubcard(originalCard->getId());
        lureTiger->setSkillName(objectName());
        lureTiger->setShowSkill(objectName());
        return lureTiger;
    }
};

class Diaolue : public TriggerSkill {
public:
    Diaolue() : TriggerSkill("diaolue") {
        events << CardFinished;
        frequency = Frequent;
        view_as_skill = new DiaolueVS;
    }

    virtual bool canPreshow() const {
        return true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const {
        if (!TriggerSkill::triggerable(player))
            return QStringList();

        if(data.value<CardUseStruct>().card->objectName()=="lure_tiger")
            return QStringList(objectName());
        else return QStringList();
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        return room->askForChoice(player, "diaolue_draw", "yes+no", QVariant(), "@diaolue-draw") == "yes";
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->drawCards(1, objectName());
        return false;
    }
};

class Xiaohan : public TriggerSkill {
public:
    Xiaohan() : TriggerSkill("xiaohan") {
        events << CardUsed << DamageCaused;
        frequency = NotFrequent;
    }

    virtual bool canPreshow() const {
        return true;
    }

    virtual QStringList triggerable(TriggerEvent event, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const {
        if(!TriggerSkill::triggerable(player)) {
            return QStringList();
        }

        if(event == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->objectName() == "slash") {
                ThunderSlash *thunder_slash = new ThunderSlash(use.card->getSuit(), use.card->getNumber());
                thunder_slash->copyFrom(use.card);
                thunder_slash->setSkillName(objectName());
                bool can_use = true;
                foreach (ServerPlayer *p, use.to) {
                    if (!player->canSlash(p, thunder_slash, false)) {
                        can_use = false;
                        break;
                    }
                }
                if (can_use)
                    return QStringList(objectName());
            }
        }else if(event == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (TriggerSkill::triggerable(player) && damage.nature == DamageStruct::Thunder && !damage.to->isNude()) {
                return QStringList(objectName());
            }
        }
        //修改闪电的伤害来源部分放在闪电的定义中实现。
        //因为闪电没有伤害来源，不会触发ConfirmDamage事件，最早只能使用DamageForseen来实现，实现不准确，故放弃

        return QStringList();
    }

    virtual bool cost(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if(event == CardUsed) {
            if (player->askForSkillInvoke(this, data)) {
                room->broadcastSkillInvoke(objectName(), player);
                return true;
            }
            return false;
        }else if(event == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            QString log = "@xiaohan-ice-sword:%1::%2";
            log = log.arg(damage.to->objectName()).arg(damage.damage);
            if(room->askForChoice(player, objectName(), "yes+no", QVariant(), log) == "yes") {
                room->broadcastSkillInvoke(objectName(), player);
                return true;
            }
        }

        return false;
    }

    virtual bool effect(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if(event == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            ThunderSlash *thunder_slash = new ThunderSlash(use.card->getSuit(), use.card->getNumber());
            thunder_slash->copyFrom(use.card);
            use.card = thunder_slash;
            data = QVariant::fromValue(use);
            room->setCardEmotion(player, thunder_slash);
        }else if(event == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            room->setEmotion(player, "weapon/ice_sword");
            if (damage.from->canDiscard(damage.to, "he")) {
                int card_id = room->askForCardChosen(player, damage.to, "he", "IceSword", false, Card::MethodDiscard);
                room->throwCard(Sanguosha->getCard(card_id), damage.to, damage.from);

                if (damage.from->isAlive() && damage.to->isAlive() && damage.from->canDiscard(damage.to, "he")) {
                    card_id = room->askForCardChosen(player, damage.to, "he", "IceSword", false, Card::MethodDiscard);
                    room->throwCard(Sanguosha->getCard(card_id), damage.to, damage.from);
                }
            }
            return true;
        }
        return false;
    }
};

class Miyu : public PhaseChangeSkill {
public:
    Miyu() : PhaseChangeSkill("miyu") {
        frequency = NotFrequent;
    }

    virtual bool canPreshow() const {
        return true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (!PhaseChangeSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Finish) {
            foreach (ServerPlayer *mate, room->getAlivePlayers()) {
                if (mate->isFriendWith(player) && mate->isWounded()) {
                    return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        int x = 0;
        foreach (ServerPlayer *mate, room->getAlivePlayers()) {
            if (player->isFriendWith(mate) && mate->isWounded()) {
                x++;
            }
        }

        if(x) {
            QList<ServerPlayer *> targets = room->askForPlayersChosen(player, room->getOtherPlayers(player,false),objectName(),0,x,QString("@miyu:::%1").arg(x),true);
            if(targets.length() > 0) {
                player->tag["miyu-targets"] = QVariant::fromValue(targets);
                room->broadcastSkillInvoke(objectName(), player);
                return true;
            }
        }

        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const {
        QList<ServerPlayer *> targets = player->tag["miyu-targets"].value< QList<ServerPlayer *> >();
        player->tag.remove("miyu-targets");
        if(targets.length() > 0) {
            Room* room = player->getRoom();
            foreach(ServerPlayer *target, targets) {
                if(!target->isAlive()) continue;

                JudgeStruct judge;
                judge.pattern = ".|spade";
                judge.good = false;
                judge.negative = true;
                judge.reason = objectName();
                judge.who = target;

                room->judge(judge);

                if(judge.isBad()) {
                    room->damage(DamageStruct(objectName(), player, target, 1, DamageStruct::Thunder));
                }
            }
        }
        return false;
    }
};

class YingzhouViewAsSkill : public OneCardViewAsSkill {
public:
    static QString mark_flag;
    static QString mark_cardid;

    YingzhouViewAsSkill() : OneCardViewAsSkill("yingzhou") {
        response_or_use = true;
    }
    bool isEnabledAtPlay(const Player *player) const {
        if(player->getMark(mark_flag) > 0) {
            //int id = player->getMark(mark_cardid);
            //Card *card = Sanguosha->getCard(id);
            Card *card = (Card*) (Self->getMark(mark_cardid));
            if(card && card->isAvailable(player)) return true;
        }
        return false;
    }
    bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if(player->getPhase() == Player::Play && player->getMark(mark_flag) > 0) {
            //int id = player->getMark(mark_cardid);
            //Card *card = Sanguosha->getCard(id);
            Card *card = (Card*) (Self->getMark(mark_cardid));
            if(card) {
                QString card_name = card->objectName();
                return pattern.contains(card_name) || card_name.contains(pattern);
            }
        }
        return false;
    }
    bool isEnabledAtNullification(const ServerPlayer *player) const {
        if(player->getPhase() == Player::Play && player->getMark(mark_flag) > 0) {
            //int id = player->getMark(mark_cardid);
            //Card *card = Sanguosha->getCard(id);
            Card *card = (Card*) (Self->getMark(mark_cardid));
            return card && card->objectName().contains("nullification");
        }
        return false;
    }
    bool viewFilter(const Card *card) const {
        if (card->isEquipped() || card->getColor()==Card::Colorless)
            return false;

        //int id = Self->getMark(mark_cardid);
        //Card *yingzhou_card = Sanguosha->getCard(id);
        Card *yingzhou_card = (Card*) (Self->getMark(mark_cardid));
        if(yingzhou_card) {
            return card->getColor() != yingzhou_card->getColor();
        }
        return false;
    }
    const Card *viewAs(const Card *originalCard) const {

        //Card* acard = Sanguosha->cloneCard(Sanguosha->getCard(Self->getMark(mark_cardid))->objectName(),
        //        originalCard->getSuit(), originalCard->getNumber());
        Card* acard = Sanguosha->cloneCard(((Card*) (Self->getMark(mark_cardid)))->objectName(),
                originalCard->getSuit(), originalCard->getNumber());

        acard->addSubcard(originalCard->getId());
        acard->setSkillName(objectName());
        acard->setShowSkill(objectName());
        acard->setCanRecast(false);
        return acard;
    }
};

class Yingzhou : public TriggerSkill {
public:
    Yingzhou() : TriggerSkill("yingzhou") {
        events << CardUsed;
        view_as_skill = new YingzhouViewAsSkill;
        frequency = NotFrequent;
    }

    virtual bool canPreshow() const {
        return false;
    }

    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();

        if(player->getPhase() == Player::Play) {
            const Card *card = data.value<CardUseStruct>().card;

            QString mark_tip = player->tag["yingzhou_mark_tip"].value<QString>();
            bool is_mark_tip_not_null = mark_tip != NULL && !mark_tip.isNull() && !mark_tip.isEmpty();

            if(card) {
                if(card->isKindOf("SkillCard")) {}
                else if(card && (card->isNDTrick() || card->isKindOf("BasicCard"))
                         && (card->getSkillName().isNull() || card->getSkillName()!=objectName())
                        && !(card->getColor()==Card::Colorless)) {
                     room->setPlayerMark(player, YingzhouViewAsSkill::mark_flag, 1);
                     //room->setPlayerMark(player, YingzhouViewAsSkill::mark_cardid, card->getId());
                     Card *cloneCard = Sanguosha->cloneCard(card);
                     player->tag["yingzhou_card"] = QVariant::fromValue(cloneCard);
                     room->setPlayerMark(player, YingzhouViewAsSkill::mark_cardid, (int)cloneCard);

                     if(player->hasShownSkill(objectName())) {
                         if(is_mark_tip_not_null)
                             room->setPlayerMark(player, mark_tip, 0);
                         mark_tip = "##yingzhou_tip+"+card->objectName()+"-Clear";
                         player->tag["yingzhou_mark_tip"] = QVariant::fromValue(mark_tip);
                         room->setPlayerMark(player, mark_tip, 1);
                     }
                } else {
                    room->setPlayerMark(player, YingzhouViewAsSkill::mark_flag, 0);

                    if(is_mark_tip_not_null)
                        room->setPlayerMark(player, mark_tip, 0);
                }
            }
        }
        return QStringList();
    }
};

QString YingzhouViewAsSkill::mark_flag =  "yingzhou_flag-Clear";
QString YingzhouViewAsSkill::mark_cardid = "yingzhou_cardid-Clear";

QifengCard::QifengCard() {}
bool QifengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const {
    return targets.length() < 1 && !to_select->hasFlag("QifengOriginal");
}
void QifengCard::onEffect(const CardEffectStruct &effect) const {
    Room *room = effect.from->getRoom();
    DamageStruct damage = room->getTag("QifengDamage").value<DamageStruct>();

    DamageStruct damage2(damage.reason, damage.from, effect.to, damage.damage, damage.nature);
    damage2.transfer = true;
    damage2.transfer_reason = "qifeng";

    room->damage(damage2);

    effect.to->drawCards(effect.to->getLostHp(), "qifeng");
}
class QifengViewAsSkill : public OneCardViewAsSkill {
public:
    QifengViewAsSkill() : OneCardViewAsSkill("qifeng") {
        filter_pattern = ".|spade|.|hand!";
        response_pattern = "@@qifeng";
    }
    const Card *viewAs(const Card *originalCard) const {
        QifengCard *card = new QifengCard;
        card->addSubcard(originalCard);
        return card;
    }
};
class Qifeng : public TriggerSkill {
public:
    Qifeng() : TriggerSkill("qifeng") {
        events << DamageCaused;
        view_as_skill = new QifengViewAsSkill;
        frequency = NotFrequent;
    }

    virtual bool canPreshow() const {
        return true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const {
        if (!TriggerSkill::triggerable(player))
            return QStringList();

        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to != player && !(damage.transfer == true && damage.transfer_reason == objectName())) {
            return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        DamageStruct damage = data.value<DamageStruct>();

        room->setTag("QifengDamage", data);
        room->setPlayerFlag(damage.to, "QifengOriginal");
        const Card* card = room->askForUseCard(player, "@@qifeng", QString("@qifeng-card:%1").arg(damage.to->objectName()), -1, Card::MethodDiscard);
        room->setPlayerFlag(damage.to, "-QifengOriginal");
        if(card != NULL) {
            room->broadcastSkillInvoke(objectName(), player);
            return true;
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const {
        return true;
    }
};

class Dianjiang : public TriggerSkill {
public:
    static QString skill_heart;
    static QString skill_diamond;
    static QString skill_spade;
    static QString skill_club;

    Dianjiang() : TriggerSkill("dianjiang") {
        events << CardsMoveOneTime << EventLoseSkill << EventAcquireSkill << GeneralShown << GeneralHidden << DFDebut;
        frequency = Compulsory;
    }

    virtual bool canPreshow() const {
        return false;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *, QVariant &, ServerPlayer* &) const {
        return QStringList();
    }

    //event是此次事件，player是事件主角（未必是拥有此技能的人），data是相关数据
    virtual void record(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const {
        if (player == NULL) return;

        bool flag = false;
        if(event == TriggerEvent::CardsMoveOneTime) {
            //如果是牌的移动，筛选条件：1.拥有技能"点绛"。2.牌是置入装备区或离开装备区。
            if(player->hasShownSkill(objectName())) {
                QVariantList move_datas = data.toList();
                foreach (QVariant move_data, move_datas) {
                    CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                    if (move.to && move.to == player && move.to_place == Player::PlaceEquip) {
                        flag = true;
                        break;
                    }

                    if (move.from && move.from == player && move.from_places.contains(Player::PlaceEquip)) {
                        flag = true;
                        break;
                    }
                }
            }
        }else flag = true;

        if(!flag) return;

        //此技能是否在副将
        bool flag_deputy = player->inDeputySkills(objectName());

        //如果此技能未亮，且拥有此技能，则清除附属技能
        if(!player->hasShownSkill(objectName())) {
            if(player->hasSkill(objectName())) {
                room->handleAcquireDetachSkills(player, getHandleString(skill_heart, flag_deputy, false));
                room->handleAcquireDetachSkills(player, getHandleString(skill_diamond, flag_deputy, false));
                room->handleAcquireDetachSkills(player, getHandleString(skill_spade, flag_deputy, false));
                room->handleAcquireDetachSkills(player, getHandleString(skill_club, flag_deputy, false));
            }
            return;
        }

        //如果此技能已亮，依据装备获得技能
        bool flag_heart = false;
        bool flag_diamond = false;
        bool flag_spade = false;
        bool flag_club = false;

        foreach (const Card *equip, player->getEquips()) {
            Card::Suit suit = equip->getSuit();
            if(equip->getSkillName().contains("hongyan")) {
                //如果装备被“红颜”影响，则只看原始花色
                suit = Sanguosha->getEngineCard(equip->getId())->getSuit();
            }

            if(suit == Card::Heart) {
                flag_heart = true;
            }else if(suit == Card::Diamond) {
                flag_diamond = true;
            }else if(suit == Card::Spade) {
                flag_spade = true;
            }else if(suit == Card::Club) {
                flag_club = true;
            }
        }

        //黑桃装备的判断比较特殊，如果存在黑桃装备的同时，还存在红桃装备，则会获得“红颜”，而“红颜”又会将黑桃装备变为红桃，这就相当于没有黑桃装备。
        flag_spade = !flag_heart && flag_spade;

        room->handleAcquireDetachSkills(player, getHandleString(skill_heart, flag_deputy, flag_heart));
        room->handleAcquireDetachSkills(player, getHandleString(skill_diamond, flag_deputy, flag_diamond));
        room->handleAcquireDetachSkills(player, getHandleString(skill_spade, flag_deputy, flag_spade));
        room->handleAcquireDetachSkills(player, getHandleString(skill_club, flag_deputy, flag_club));
    }

    static QString getHandleString(QString skill_name, bool deputy=false, bool attach=false) {
        QString str = skill_name;
        if(deputy) str.append('!');
        if(!attach) str = "-" + str;
        return str;
    }
};

QString Dianjiang::skill_heart = "hongyan";
QString Dianjiang::skill_diamond = "qiaopo";
QString Dianjiang::skill_spade = "xuanyang_liyunpeng";
QString Dianjiang::skill_club = "biyue_liyunpeng";

YijiuCard::YijiuCard() {
    mute = true;
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}
void YijiuCard::onUse(Room *room, const CardUseStruct &use) const {
    ServerPlayer* player = use.from;
    room->setPlayerMark(player, "@yijiu", 0);
    room->broadcastSkillInvoke("yijiu", player);
    room->doSuperLightbox("liyunpeng", "yijiu");

    CardUseStruct new_use = use;
    new_use.to << player;

    Card::onUse(room, new_use);
}
void YijiuCard::onEffect(const CardEffectStruct &effect) const {
    effect.from->addToPile("jiuyou", effect.card->getSubcards()[0], true);
}

class Yijiu : public OneCardViewAsSkill {
public:
    static QString yijiu_pile;
    static QString skill_heart;
    static QString skill_diamond;
    static QString skill_spade;
    static QString skill_club;

    Yijiu() : OneCardViewAsSkill("yijiu")
    {
        frequency = Limited;
        limit_mark = "@yijiu";
        filter_pattern = ".!";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        YijiuCard *yijiuCard = new YijiuCard;
        yijiuCard->addSubcard(originalCard);
        yijiuCard->setShowSkill(objectName());
        return yijiuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@yijiu") >= 1;
    }
};

QString Yijiu::yijiu_pile = "jiuyou";
QString Yijiu::skill_heart = "tiancheng";
QString Yijiu::skill_diamond = "lianji";
QString Yijiu::skill_spade = "huixia_liyunpeng";
QString Yijiu::skill_club = "liangcai_liyunpeng";

class YijiuSkillKeep : public TriggerSkill {
public:

    YijiuSkillKeep() : TriggerSkill("#yijiuskillkeep") {
        events << CardsMoveOneTime << EventLoseSkill << EventAcquireSkill << GeneralShown << GeneralHidden << DFDebut;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *, QVariant &, ServerPlayer* &) const {
        return QStringList();
    }

    //event是此次事件，player是事件主角（未必是拥有此技能的人），data是相关数据
    virtual void record(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const {
        if (player == NULL) return;

        bool flag = false;
        if(event == TriggerEvent::CardsMoveOneTime) {
            //如果是牌的移动，筛选条件：1.拥有此技能。2.牌是置入“旧游”或离开“旧游”。
            if(player->hasShownSkill(objectName())) {
                QVariantList move_datas = data.toList();
                foreach (QVariant move_data, move_datas) {
                    CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                    if (move.to && move.to == player && move.origin_to_pile_name==Yijiu::yijiu_pile) {
                        flag = true;
                        break;
                    }

                    if (move.from && move.from == player && move.origin_from_pile_names.contains(Yijiu::yijiu_pile)) {
                        flag = true;
                        break;
                    }
                }
            }
        }else flag = true;

        if(!flag) return;

        //此技能是否在副将
        bool flag_deputy = player->inDeputySkills(objectName());

        //如果此技能未亮，且拥有此技能，则清除附属技能
        if(!player->hasShownSkill(objectName())) {
            if(player->hasSkill(objectName())) {
                room->handleAcquireDetachSkills(player, Dianjiang::getHandleString(Yijiu::skill_heart, flag_deputy, false));
                room->handleAcquireDetachSkills(player, Dianjiang::getHandleString(Yijiu::skill_diamond, flag_deputy, false));
                room->handleAcquireDetachSkills(player, Dianjiang::getHandleString(Yijiu::skill_spade, flag_deputy, false));
                room->handleAcquireDetachSkills(player, Dianjiang::getHandleString(Yijiu::skill_club, flag_deputy, false));
            }
            return;
        }

        //如果此技能已亮，依据“旧游”获得技能
        bool flag_heart = false;
        bool flag_diamond = false;
        bool flag_spade = false;
        bool flag_club = false;

        foreach (int cardID, player->getPile(Yijiu::yijiu_pile)) {
            const Card* card = Sanguosha->getCard(cardID);
            Card::Suit suit = card->getSuit();

            if(suit == Card::Heart) {
                flag_heart = true;
            }else if(suit == Card::Diamond) {
                flag_diamond = true;
            }else if(suit == Card::Spade) {
                flag_spade = true;
            }else if(suit == Card::Club) {
                flag_club = true;
            }
        }

        room->handleAcquireDetachSkills(player, Dianjiang::getHandleString(Yijiu::skill_heart, flag_deputy, flag_heart));
        room->handleAcquireDetachSkills(player, Dianjiang::getHandleString(Yijiu::skill_diamond, flag_deputy, flag_diamond));
        room->handleAcquireDetachSkills(player, Dianjiang::getHandleString(Yijiu::skill_spade, flag_deputy, flag_spade));
        room->handleAcquireDetachSkills(player, Dianjiang::getHandleString(Yijiu::skill_club, flag_deputy, flag_club));
    }
};

class Xuanyang : public TriggerSkill {
public:
    Xuanyang() : TriggerSkill("xuanyang") {
        events << TargetChosen;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!TriggerSkill::triggerable(player))
            return QStringList();

        if (use.card != NULL && use.card->isKindOf("Slash") && use.to.length() == 1) {
            ServerPlayer *target = use.to.at(use.index);
            if (target != NULL)
                return QStringList(objectName() + "->" + target->objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who != NULL && ask_who->askForSkillInvoke(this, QVariant::fromValue(skill_target))) {
            room->setEmotion(ask_who, "weapon/double_sword");
            room->broadcastSkillInvoke(objectName(), ask_who);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &data, ServerPlayer *ask_who) const {
        bool draw_card = false;
        if (!skill_target->canDiscard(skill_target, "h"))
            draw_card = true;
        else {
            if(!room->askForCard(skill_target, ".|.|.|hand", "@xuanyang-discard:" + ask_who->objectName(), data, Card::MethodDiscard))
                draw_card = true;
        }
        if (draw_card)
            ask_who->drawCards(1);
        return false;
    }
};

class Huixia : public TriggerSkill {
public:
    Huixia() : TriggerSkill("huixia") {
        events << SlashMissed;
    }

    virtual bool canPreshow() const {
        return true;
    }

    virtual TriggerList triggerable(TriggerEvent , Room *, ServerPlayer *, QVariant &data) const {
        TriggerList skill_list;
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        QMap<ServerPlayer *, const Card *> kexins;

        if(effect.to->hasSkill(this)) {
            kexins.insert(effect.to, effect.slash);
        }

        if(effect.from->hasSkill(this)) {
            kexins.insert(effect.from, effect.jink);
        }

        for(ServerPlayer* kexin : kexins.keys()) {
            const Card* card = kexins.value(kexin);
            if(card) {
                if(!card->isVirtualCard() || (card->isVirtualCard() && card->getSubcards().length()>0))
                    skill_list.insert(kexin, QStringList(objectName()));
            }
        }

        return skill_list;

    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *kexin) const {
       if (kexin->askForSkillInvoke(this, data)) {
            room->broadcastSkillInvoke(objectName(), kexin);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *kexin) const {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        if(kexin == effect.to) {
            kexin->obtainCard(effect.slash);
        }

        if(kexin == effect.from) {
            kexin->obtainCard(effect.jink);
        }

        return false;
    }
};

class Liangcai : public TriggerSkill {
public:
    Liangcai() : TriggerSkill("liangcai") {
        events << AskForRetrial;
    }

    virtual bool canPreshow() const {
        return true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer * &) const {
        if (!TriggerSkill::triggerable(player))
            return QStringList();

        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(this, data)) {
            room->notifySkillInvoked(player, objectName());
            room->broadcastSkillInvoke(objectName(), player);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        const Card* card = Sanguosha->getCard(room->getNCards(1)[0]);

        CardMoveReason reason(CardMoveReason::S_REASON_RESPONSE, player->objectName(), objectName(), QString());
        room->moveCardTo(card, NULL, Player::PlaceTable, reason);

        JudgeStruct* judge = data.value<JudgeStruct*>();
        room->retrial(card, player, judge, objectName());
        judge->updateResult();

        return false;
    }
};

class Erze : public TriggerSkill {
public:
    Erze() : TriggerSkill("erze") {
        events << SlashMissed << DamageCaused;
    }

    virtual bool canPreshow() const {
        return true;
    }

    virtual QStringList triggerable(TriggerEvent event, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &player2) const {
        if(event == SlashMissed) {
            if(!TriggerSkill::triggerable(player2)) return QStringList();

            if(!data.value<SlashEffectStruct>().to->isAlive()) return QStringList();
            int redCount = 0;
            int blackCount = 0;
            foreach (const Card *card, player2->getHandcards()) {
                if (card->isRed()) redCount++;
                else if (card->isBlack()) blackCount++;
            }
            if (redCount >= blackCount) return QStringList(objectName());
        }else if(event == DamageCaused) {
            if (!TriggerSkill::triggerable(player)) return QStringList();
            DamageStruct damage = data.value<DamageStruct>();
            if(damage.card != NULL && damage.card->isKindOf("Slash") && !damage.chain && !damage.transfer) {
                int redCount = 0;
                int blackCount = 0;
                foreach (const Card *card, player->getHandcards()) {
                    if (card->isRed()) redCount++;
                    else if (card->isBlack()) blackCount++;
                }
                if (blackCount > redCount) return QStringList(objectName());
            }
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *player2) const {
        ServerPlayer *xiaoyue = player;
        ServerPlayer *target = player;
        if(event == SlashMissed)  {
            xiaoyue = player2;
            target = data.value<SlashEffectStruct>().to;
        }else if(event == DamageCaused){
            xiaoyue = player;
            target = data.value<DamageStruct>().to;
        }
        if (xiaoyue->askForSkillInvoke(this, QVariant::fromValue(target))) {
            room->notifySkillInvoked(xiaoyue, objectName());
            room->broadcastSkillInvoke(objectName(), xiaoyue);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *player2) const {
        ServerPlayer *xiaoyue = player;
        if(event == SlashMissed) xiaoyue = player2;
        room->showAllCards(xiaoyue);
        if(event == SlashMissed) {
            room->setEmotion(xiaoyue, "weapon/axe");
            room->slashResult(data.value<SlashEffectStruct>(), NULL);
        }else if(event == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            damage.damage++;
            data = QVariant::fromValue(damage);

            room->setEmotion(xiaoyue, "weapon/guding_blade");
        }

        return false;
    }
};

class Shenglong : public TriggerSkill {
public:
    Shenglong() : TriggerSkill("shenglong") {
        frequency = Compulsory;
    }

    virtual bool canPreshow() const {
        return false;
    }

    //功能实现在 Player::canSlash() 方法中
};

YunPackage::YunPackage()
    :Package("yun")
{
    General *huaibeibei = new General(this, "huaibeibei", "wu", 4, false); // G.Yun 001
    huaibeibei->addCompanion("hanjing");
    huaibeibei->addSkill(new Zhuyan);
    huaibeibei->addSkill(new Tiancheng);
    huaibeibei->addSkill(new Tianchengkeep);
    insertRelatedSkills("tiancheng", "#tianchengkeep");
    insertRelatedSkills("hongyan_huaibeibei", "#hongyan_huaibeibei-maxcards");
    huaibeibei->addRelateSkill("hongyan_huaibeibei");

    General *hanjing = new General(this, "hanjing", "wu", 3, false); // G.Yun 002
    hanjing->addSkill(new Lianji);
    hanjing->addSkill(new Qiaopo);

    General *wangcan = new General(this, "wangcan", "wei", 3, false); // G.Yun 003
    wangcan->addSkill(new Siwu);
    wangcan->addSkill(new SiwuDraw);
    wangcan->addSkill(new DetachEffectSkill("siwu","&wire"));
    insertRelatedSkills("siwu", 2, "#siwu-draw", "#siwu-clear");
    wangcan->addSkill(new Xingcan);

    General *xiaosa = new General(this, "xiaosa", "wei", 4, false); // G.Yun 005
    xiaosa->addSkill(new Xiaohan);
    xiaosa->addSkill(new Miyu);

    General *yangwenqi = new General(this, "yangwenqi", "shu", 4, false); // G.Yun 004
    yangwenqi->addSkill(new Zhangui);
    yangwenqi->addSkill(new Diaolue);

    General *lishuyu = new General(this, "lishuyu", "shu", 3, false); // G.Yun 006
    lishuyu->addSkill(new Yingzhou);
    lishuyu->addSkill(new Qifeng);

    General *liyunpeng = new General(this, "liyunpeng", "qun", 3); // G.Yun 007
    liyunpeng->addSkill(new Dianjiang);
    liyunpeng->addSkill(new Yijiu);
    liyunpeng->addSkill(new YijiuSkillKeep);
    insertRelatedSkills("yijiu", "#yijiuskillkeep");
    liyunpeng->addRelateSkill(Dianjiang::skill_heart);
    liyunpeng->addRelateSkill(Dianjiang::skill_diamond);
    liyunpeng->addRelateSkill(Dianjiang::skill_spade);
    liyunpeng->addRelateSkill(Dianjiang::skill_club);
    liyunpeng->addRelateSkill(Yijiu::skill_heart);
    liyunpeng->addRelateSkill(Yijiu::skill_diamond);
    liyunpeng->addRelateSkill(Yijiu::skill_spade);
    liyunpeng->addRelateSkill(Yijiu::skill_club);
    Xuanyang* xuanyang_liyunpeng = new Xuanyang;
    xuanyang_liyunpeng->setObjectName(Dianjiang::skill_spade);
    Huixia* huixia_liyunpeng = new Huixia;
    huixia_liyunpeng->setObjectName(Yijiu::skill_spade);
    Liangcai* liangcai_liyunpeng = new Liangcai;
    liangcai_liyunpeng->setObjectName(Yijiu::skill_club);
    Biyue* biyue_liyunpeng = new Biyue;
    biyue_liyunpeng->setObjectName(Dianjiang::skill_club);

    General *congxiaoyue = new General(this, "congxiaoyue", "qun", 4); //G.YUN 008
    congxiaoyue->addSkill(new Erze);
    congxiaoyue->addSkill(new Shenglong);
    congxiaoyue->addCompanion("liyunpeng");

    General *likexin = new General(this, "likexin", "qun", 3, false, true); // G.Yun 007.SP01
    likexin->addSkill(new Xuanyang);
    likexin->addSkill(new Huixia);
    likexin->addCompanion("liyunpeng");

    General *liujianan = new General(this, "liujianan", "qun", 4, false, true); // G.Yun 007.SP02
    liujianan->addSkill(new Liangcai);
    Biyue* biyue_liujianan = new Biyue;
    biyue_liujianan->setObjectName("biyue_liujianan");
    liujianan->addSkill(biyue_liujianan);

    addMetaObject<QiaopoCard>();
    addMetaObject<QifengCard>();
    addMetaObject<YijiuCard>();

    skills << new HongyanHuaibeibei << new HongyanHuaibeibeiMaxCards << xuanyang_liyunpeng << huixia_liyunpeng << liangcai_liyunpeng << biyue_liyunpeng;
}

ADD_PACKAGE(Yun)

