#include "settings.h"
#include "standard-basics.h"
#include "skill.h"
#include "yun.h"
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
        bool has_head_zhuyan = false;
        bool has_deputy_hongyan = false;
        bool has_deputy_zhuyan = false;

        foreach (const Skill *skill, player->getHeadSkillList(true, true)) {
            if (skill->objectName() == "hongyan") {
                has_head_hongyan = true;
            }else if (skill->objectName() == "zhuyan") {
                has_head_zhuyan = true;
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
    static QString mark_times;

     Tiancheng() : TriggerSkill("tiancheng") {
         events << CardResponded << CardUsed << FinishJudge;
         frequency = Frequent;
     }

     virtual QStringList triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
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
         int times = player->getMark(mark_times);
         if(player->getPhase() == Player::Play) {
             if(player->getMark(mark_times)) {
                 room->askForDiscard(player, objectName(), 1, 1, false, true);

                 QString mark_tip = player->tag["tiancheng_mark_tip"].value<QString>();
                 if(mark_tip != NULL && !mark_tip.isNull() && !mark_tip.isEmpty())
                    room->setPlayerMark(player, mark_tip, 0);
                 mark_tip = QString("##tiancheng_tip+")+QString::number(times)+"-Clear";
                 room->setPlayerMark(player, mark_tip, 1);
                 player->tag["tiancheng_mark_tip"] = QVariant::fromValue(mark_tip);
             }
             room->setPlayerMark(player, mark_times, times+1);
         }

         return false;
     }
};

class tianchengkeep : public MaxCardsSkill {
public:
    tianchengkeep() : MaxCardsSkill("#tianchengkeep") {
        frequency = Compulsory;
    }

    int getExtra(const Player *target) const {
        if (target->hasShownSkill("tiancheng")) {
            int keep = target->getMark(Tiancheng::mark_times)-1;
            if(keep < 0) keep = 0;
            return keep;
        } else return 0;
    }
};

QString Tiancheng::mark_times = "tiancheng-Clear";

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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (TriggerSkill::triggerable(player) && !player->isKongcheng() &&
                !(player->hasFlag("qiaopo1used") && player->hasFlag("qiaopo2used"))) {
            player->tag["QiaopoDamage"] = data;
            QStringList trigger_skill;
            for(int i = 0; i < data.value<DamageStruct>().damage;i++) {
                trigger_skill << objectName();
            }
            return trigger_skill;
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if(room->askForUseCard(player, "@@qiaopo", "@qiaopo-card"))
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *) const {
        DamageStruct damage = data.value<DamageStruct>();

        --damage.damage;
        if (damage.damage < 1)
            return true;
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
        if (from->hasShownSkill(objectName()))
            return 1024;
        else return 0;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const {
        if (from->hasShownSkill(objectName()) && from->getSlashCount() == 0)
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

    virtual bool canPreshow() const {
        return true;
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
        if (!TriggerSkill::triggerable(player))
            return QStringList();

        if(event == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->objectName() == "slash" && !use.card->isVirtualCard()) {
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
            QString log = "@xiaohan-ice_sword:%1::%2";
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (!PhaseChangeSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Finish) {
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        QList<const Player *> players = player->getAliveSiblings();
        players << player;

        int x = 0;
        foreach (const Player *p, players) {
            if (player->isFriendWith(p) && p->isWounded()) {
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
            int id = player->getMark(mark_cardid);
            Card *card = Sanguosha->getCard(id);
            if(card && card->isAvailable(player)) return true;
        }
        return false;
    }
    bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if(player->getPhase() == Player::Play && player->getMark(mark_flag) > 0) {
            int id = player->getMark(mark_cardid);
            Card *card = Sanguosha->getCard(id);
            if(card) {
                QString card_name = card->objectName();
                return pattern.contains(card_name) || card_name.contains(pattern);
            }
        }
        return false;
    }
    bool isEnabledAtNullification(const ServerPlayer *player) const {
        if(player->getPhase() == Player::Play && player->getMark(mark_flag) > 0) {
            int id = player->getMark(mark_cardid);
            Card *card = Sanguosha->getCard(id);
            return card && card->objectName().contains("nullification");
        }
        return false;
    }
    bool viewFilter(const Card *card) const {
        if (card->isEquipped() || card->getColor()==Card::Colorless)
            return false;

        int id = Self->getMark(mark_cardid);
        Card *yingzhou_card = Sanguosha->getCard(id);
        if(yingzhou_card) {
            return card->getColor() != yingzhou_card->getColor();
        }
        return false;
    }
    const Card *viewAs(const Card *originalCard) const {
        Card* acard = Sanguosha->cloneCard(Sanguosha->getCard(Self->getMark(mark_cardid))->objectName(),
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

    virtual QStringList triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();

        if(player->getPhase() == Player::Play) {
            const Card *card = data.value<CardUseStruct>().card;

            QString mark_tip = player->tag["yingzhou_mark_tip"].value<QString>();
            bool is_mark_tip_not_null = mark_tip != NULL && !mark_tip.isNull() && !mark_tip.isEmpty();

            if(card) {
                if(card->isKindOf("SkillCard")) {}
                else if(card && (card->isNDTrick() || card->isKindOf("BasicCard"))
                         && !(card->getSkillName()==objectName()) && !(card->getColor()==Card::Colorless)) {
                     room->setPlayerMark(player, YingzhouViewAsSkill::mark_flag, 1);
                     room->setPlayerMark(player, YingzhouViewAsSkill::mark_cardid, card->getId());

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
        const Card* res = room->askForUseCard(player, "@@qifeng", QString("@qifeng-card:%1").arg(damage.to->objectName()), -1, Card::MethodDiscard);
        room->setPlayerFlag(damage.to, "-QifengOriginal");
        if(res) {
            room->broadcastSkillInvoke(objectName());
        }

        return res;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const {
        return true;
    }
};

YunPackage::YunPackage()
    :Package("yun")
{
    General *huaibeibei = new General(this, "huaibeibei", "wu", 4, false); // G.Yun 001
    huaibeibei->addCompanion("hanjing");
    huaibeibei->addSkill(new Zhuyan);
    huaibeibei->addSkill(new Tiancheng);
    huaibeibei->addSkill(new tianchengkeep);
    related_skills.insertMulti("tiancheng", "#tianchengkeep");
    related_skills.insertMulti("hongyan_huaibeibei", "#hongyan_huaibeibei-maxcards");
    huaibeibei->addRelateSkill("hongyan_huaibeibei");

    General *hanjing = new General(this, "hanjing", "wu", 3, false); // G.Yun 002
    hanjing->addSkill(new Lianji);
    hanjing->addSkill(new Qiaopo);

    General *wangcan = new General(this, "wangcan", "wei", 3, false); // G.Yun 003
    wangcan->addSkill(new Siwu);
    wangcan->addSkill(new SiwuDraw);
    wangcan->addSkill(new Xingcan);
    related_skills.insertMulti("siwu", "#siwu-draw");

    General *yangwenqi = new General(this, "yangwenqi", "shu", 4, false); // G.Yun 004
    yangwenqi->addSkill(new Zhangui);
    yangwenqi->addSkill(new Diaolue);

    General *xiaosa = new General(this, "xiaosa", "wei", 4, false); // G.Yun 005
    xiaosa->addSkill(new Xiaohan);
    xiaosa->addSkill(new Miyu);

    General *lishuyu = new General(this, "lishuyu", "shu", 3, false); // G.Yun 006
    lishuyu->addSkill(new Yingzhou);
    lishuyu->addSkill(new Qifeng);

    /*General *liyunpeng = new General(this, "liyunpeng", "qun", 4); // G.Yun 007
    liyunpeng->setDeputyMaxHpAdjustedValue(-1);*/

    addMetaObject<QiaopoCard>();
    addMetaObject<QifengCard>();

    skills << new HongyanHuaibeibei << new HongyanHuaibeibeiMaxCards;
}

ADD_PACKAGE(Yun)

