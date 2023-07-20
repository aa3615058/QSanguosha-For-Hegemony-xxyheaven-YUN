#ifndef YUN_H
#define YUN_H

#include "package.h"
#include "card.h"
#include "skill.h"

class YunPackage : public Package
{
    Q_OBJECT

public:
    YunPackage();
};

class QiaopoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QiaopoCard();

    void onEffect(const CardEffectStruct &effect) const;
};

class QifengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QifengCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

#endif // YUN_H
