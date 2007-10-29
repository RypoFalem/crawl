/*
 *  File:       it_use2.cc
 *  Summary:    Functions for using wands, potions, and weapon/armour removal.4\3
 *  Written by: Linley Henzell
 *
 *  Modified for Crawl Reference by $Author$ on $Date$
 *
 *  Change History (most recent first):
 *
 *      26jun2000   jmf   added ZAP_MAGMA
 *  <4> 19mar2000   jmf   Added ZAP_BACKLIGHT and ZAP_SLEEP
 *  <3>     10/1/99         BCR             Changed messages for speed and
 *                                          made amulet resist slow up speed
 *  <2>     5/20/99         BWR             Fixed bug with RAP_METABOLISM
 *                                          and RAP_NOISES artefacts/
 *  <1>     -/--/--         LRH             Created
 */

#include "AppHdr.h"
#include "it_use2.h"

#include <stdio.h>
#include <string.h>

#include "externs.h"

#include "beam.h"
#include "effects.h"
#include "food.h"
#include "itemname.h"
#include "itemprop.h"
#include "misc.h"
#include "mutation.h"
#include "player.h"
#include "randart.h"
#include "religion.h"
#include "skills2.h"
#include "spells2.h"
#include "spl-cast.h"
#include "spl-util.h"
#include "stuff.h"
#include "view.h"

// From an actual potion, pow == 40 -- bwr
bool potion_effect( potion_type pot_eff, int pow )
{
    bool effect = true;  // current behaviour is all potions id on quaffing

    int new_value = 0;

    if (pow > 150)
        pow = 150;

    const int factor = (you.species == SP_VAMPIRE ? 2 : 1);

    switch (pot_eff)
    {
    case POT_HEALING:

        inc_hp( (5 + random2(7)) / factor, false);
        mpr("You feel better.");

        if (you.species == SP_VAMPIRE)
        {
           if (!one_chance_in(3))
               you.rotting = 0;
           if (!one_chance_in(3))
               you.duration[DUR_CONF] = 0;
        }
        else
        {
            // only fix rot when healed to full 
            if (you.hp == you.hp_max)
            {
                unrot_hp(1);
                set_hp(you.hp_max, false);
            }
            
            you.duration[DUR_POISONING] = 0;
            you.rotting = 0;
            you.disease = 0;
            you.duration[DUR_CONF] = 0;
        }
        break;

    case POT_HEAL_WOUNDS:
        inc_hp((10 + random2avg(28, 3)) / factor, false);
        mpr("You feel much better.");

        // only fix rot when healed to full 
        if ( you.species != SP_VAMPIRE && you.hp == you.hp_max )
        {
            unrot_hp( 2 + random2avg(5, 2) );
            set_hp(you.hp_max, false);
        }
        break;

    case POT_SPEED:
        haste_player( (40 + random2(pow)) / factor );
        break;

    case POT_MIGHT:
    {
        const bool were_mighty = (you.duration[DUR_MIGHT] > 0);
        
        mprf(MSGCH_DURATION, "You feel %s all of a sudden.",
             were_mighty ? "mightier" : "very mighty");

        if ( were_mighty )
            contaminate_player(1);
        else
            modify_stat(STAT_STRENGTH, 5, true);

        // conceivable max gain of +184 {dlb}
        you.duration[DUR_MIGHT] += 35 + random2(pow);
        
        // files.cc permits values up to 215, but ... {dlb}
        if (you.duration[DUR_MIGHT] > 80)
            you.duration[DUR_MIGHT] = 80;

        did_god_conduct( DID_STIMULANTS, 4 + random2(4) );
        break;
    }

    case POT_GAIN_STRENGTH:
        mutate(MUT_STRONG);
        break;

    case POT_GAIN_DEXTERITY:
        mutate(MUT_AGILE);
        break;

    case POT_GAIN_INTELLIGENCE:
        mutate(MUT_CLEVER);
        break;

    case POT_LEVITATION:
        mprf("You feel %s buoyant.",
             (!player_is_airborne()) ? "very" : "more");

        if (!player_is_airborne())
            mpr("You gently float upwards from the floor.");

        you.duration[DUR_LEVITATION] += 25 + random2(pow);

        if (you.duration[DUR_LEVITATION] > 100)
            you.duration[DUR_LEVITATION] = 100;

        burden_change();
        break;

    case POT_POISON:
    case POT_STRONG_POISON:
        if (player_res_poison())
        {
            mprf("You feel %s nauseous.",
                 (pot_eff == POT_POISON) ? "slightly" : "quite" );
        }
        else
        {
            mprf("That liquid tasted %s nasty...",
                 (pot_eff == POT_POISON) ? "very" : "extremely" );

            poison_player( ((pot_eff == POT_POISON) ? 1 + random2avg(5, 2) 
                                                    : 3 + random2avg(13, 2)) );
            xom_is_stimulated(128);
        }
        break;

    case POT_SLOWING:
        if (slow_player((10 + random2(pow)) / factor))
            xom_is_stimulated(64 / factor);
        break;

    case POT_PARALYSIS:
        you.paralyse(2 + random2( 6 + you.duration[DUR_PARALYSIS] ));
        xom_is_stimulated(64);
        break;

    case POT_CONFUSION:
        if (confuse_player((3 + random2(8)) / factor))
            xom_is_stimulated(128 / factor);
        break;

    case POT_INVISIBILITY:
        mpr( (!you.duration[DUR_INVIS]) ? "You fade into invisibility!"
             : "You fade further into invisibility." );

        // Invisibility cancels backlight.
        you.duration[DUR_BACKLIGHT] = 0;

        // now multiple invisiblity casts aren't as good -- bwr
        if (!you.duration[DUR_INVIS])
            you.duration[DUR_INVIS] = 15 + random2(pow);
        else
            you.duration[DUR_INVIS] += random2(pow);

        if (you.duration[DUR_INVIS] > 100)
            you.duration[DUR_INVIS] = 100;

        you.duration[DUR_BACKLIGHT] = 0;
        break;

    case POT_PORRIDGE:          // oatmeal - always gluggy white/grey?
        if (you.species == SP_VAMPIRE || you.mutation[MUT_CARNIVOROUS] == 3)
        {
            mpr("Blech - that potion was really gluggy!");
        }
        else
        {
            mpr("That potion was really gluggy!");
            lessen_hunger(6000, true);
        }
        break;

    case POT_DEGENERATION:
        if ( pow == 40 )
            mpr("There was something very wrong with that liquid!");
        if (lose_stat(STAT_RANDOM, 1 + random2avg(4, 2)))
            xom_is_stimulated(64);
        break;

    // Don't generate randomly - should be rare and interesting
    case POT_DECAY:
        if (rot_player(10 + random2(10)))
            xom_is_stimulated(64);
        break;

    case POT_WATER:
        if (you.species == SP_VAMPIRE)
        {
            mpr("Blech - this tastes like water.");
        }
        else
        {
            mpr("This tastes like water.");
            lessen_hunger(20, true);
        }
        break;

    case POT_EXPERIENCE:
        if (you.experience_level < 27)
        {
            mpr("You feel more experienced!");

            you.experience = 1 + exp_needed( 2 + you.experience_level );
            level_change();
        }
        else
            mpr("A flood of memories washes over you.");
        break;                  // I'll let this slip past robe of archmagi

    case POT_MAGIC:
        mpr( "You feel magical!" );
        new_value = 5 + random2avg(19, 2);

        // increase intrinsic MP points
        if (you.magic_points + new_value > you.max_magic_points)
        {
            new_value = (you.max_magic_points - you.magic_points)
                + (you.magic_points + new_value - you.max_magic_points)/4 + 1;
        }

        inc_mp( new_value, true );
        break;

    case POT_RESTORE_ABILITIES:
        // give a message if restore_stat doesn't
        if (!restore_stat(STAT_ALL, false))
            mpr( "You feel refreshed." );
        break;

    case POT_BERSERK_RAGE:
        if (you.species == SP_VAMPIRE)
        {
            mpr("You feel slightly irritated.");
            make_hungry(100, false);
        }
        else if ( you.duration[DUR_BUILDING_RAGE] )
        {
            you.duration[DUR_BUILDING_RAGE] = 0;
            mpr("Your blood cools.");
        }
        else
        {
            if (go_berserk(true))
                xom_is_stimulated(64);
        }
        break;

    case POT_CURE_MUTATION:
        mpr("It has a very clean taste.");
        for (int i = 0; i < 7; i++)
            if (random2(10) > i)
                delete_mutation(RANDOM_MUTATION);
        break;

    case POT_MUTATION:
        mpr("You feel extremely strange.");
        for (int i = 0; i < 3; i++)
            mutate(RANDOM_MUTATION, false);
        did_god_conduct(DID_STIMULANTS, 4 + random2(4));
        break;
        
  case POT_BLOOD:
        if (you.species == SP_VAMPIRE)
        {
            int temp_rand = random2(9);
            strcpy(info, (temp_rand == 0) ? "human" :
                         (temp_rand == 1) ? "rat" :
                         (temp_rand == 2) ? "goblin" :
                         (temp_rand == 3) ? "elf" :
                         (temp_rand == 4) ? "goat" :
                         (temp_rand == 5) ? "sheep" :
                         (temp_rand == 6) ? "gnoll" :
                         (temp_rand == 7) ? "sheep"
                                          : "yak");
                                          
           mprf("Yummy - fresh %s blood!", info);

           lessen_hunger(1000, true);
           mpr("You feel better.");
           inc_hp(1 + random2(10), false);
        }
        else
        {
            bool likes_blood = (you.species == SP_OGRE
                                || you.species == SP_TROLL
                                || you.mutation[MUT_CARNIVOROUS]);

            if (likes_blood)
            {
                mpr("This tastes like blood.");
                lessen_hunger(200, true);
            }
            else
            {
                mpr("Blech - this tastes like blood!");
                if (!you.mutation[MUT_HERBIVOROUS] && one_chance_in(3))
                    lessen_hunger(100, true);
                else
                    disease_player( 50 + random2(100) );
                xom_is_stimulated(32);
            }
        }
        did_god_conduct(DID_DRINK_BLOOD, 1 + random2(3));
        break;

    case POT_RESISTANCE:
        mpr("You feel protected.");
        you.duration[DUR_RESIST_FIRE]   += random2(pow) + 10;
        you.duration[DUR_RESIST_COLD]   += random2(pow) + 10;
        you.duration[DUR_RESIST_POISON] += random2(pow) + 10;
        you.duration[DUR_INSULATION]    += random2(pow) + 10;
        // one contamination point for each resist
        contaminate_player(4);
        break;

    case NUM_POTIONS:
        mpr("You feel bugginess flow through your body.");
        break;
    }

    return (effect);
}                               // end potion_effect()

void unwield_item(bool showMsgs)
{
    const int unw = you.equip[EQ_WEAPON];
    if ( unw == -1 )
        return;
    
    you.equip[EQ_WEAPON] = -1;
    you.special_wield = SPWLD_NONE;
    you.wield_change = true;

    item_def &item(you.inv[unw]);

    if ( item.base_type == OBJ_MISCELLANY &&
         item.sub_type == MISC_LANTERN_OF_SHADOWS )
    {
        you.current_vision += 2;
        setLOSRadius(you.current_vision);
    }

    if (item.base_type == OBJ_WEAPONS)
    {
        if (is_fixed_artefact( item ))
        {
            switch (item.special)
            {
            case SPWPN_SINGING_SWORD:
                if (showMsgs)
                    mpr("The Singing Sword sighs.", MSGCH_TALK);
                break;
            case SPWPN_WRATH_OF_TROG:
                if (showMsgs)
                    mpr("You feel less violent.");
                break;
            case SPWPN_SCYTHE_OF_CURSES:
            case SPWPN_STAFF_OF_OLGREB:
                item.plus = 0;
                item.plus2 = 0;
                break;
            case SPWPN_STAFF_OF_WUCAD_MU:
                item.plus = 0;
                item.plus2 = 0;
                miscast_effect( SPTYP_DIVINATION, 9, 90, 100,
                                "the Staff of Wucad Mu" );
                break;
            default:
                break;
            }

            return;
        }

        const int brand = get_weapon_brand( item );

        if (is_random_artefact( item ))
            unuse_randart(unw);

        if (brand != SPWPN_NORMAL)
        {
            const std::string msg = item.name(DESC_CAP_YOUR);
            
            switch (brand)
            {
            case SPWPN_SWORD_OF_CEREBOV:
            case SPWPN_FLAMING:
                if (showMsgs)
                    mprf("%s stops flaming.", msg.c_str());
                break;

            case SPWPN_FREEZING:
            case SPWPN_HOLY_WRATH:
                if (showMsgs)
                    mprf("%s stops glowing.", msg.c_str());
                break;

            case SPWPN_ELECTROCUTION:
                if (showMsgs)
                    mprf("%s stops crackling.", msg.c_str());
                break;

            case SPWPN_VENOM:
                if (showMsgs)
                    mprf("%s stops dripping with poison.", msg.c_str());
                break;

            case SPWPN_PROTECTION:
                if (showMsgs)
                    mpr("You feel less protected.");
                you.redraw_armour_class = 1;
                break;

            case SPWPN_VAMPIRICISM:
                if (showMsgs)
                    mpr("You feel the strange hunger wane.");
                break;

                /* case 8: draining
                   case 9: speed, 10 slicing etc */

            case SPWPN_DISTORTION:
                // Removing the translocations skill reduction of effect,
                // it might seem sensible, but this brand is supposed
                // to be dangerous because it does large bonus damage,
                // as well as free teleport other side effects, and
                // even with the miscast effects you can rely on the
                // occasional spatial bonus to mow down some opponents.
                // It's far too powerful without a real risk, especially
                // if it's to be allowed as a player spell. -- bwr

                // int effect = 9 - random2avg( you.skills[SK_TRANSLOCATIONS] * 2, 2 );
                miscast_effect( SPTYP_TRANSLOCATION, 9, 90, 100,
                                "a distortion effect" );
                break;

                // when more are added here, *must* duplicate unwielding
                // effect in vorpalise weapon scroll effect in read_scoll
            }                   // end switch

            if (you.duration[DUR_WEAPON_BRAND])
            {
                you.duration[DUR_WEAPON_BRAND] = 0;
                set_item_ego_type( item, OBJ_WEAPONS, SPWPN_NORMAL );
                // we're letting this through even if hiding messages
                mpr("Your branding evaporates.");
            }
        }                       // end if
    }

    if (item.base_type == OBJ_STAVES && item.sub_type == STAFF_POWER)
        calc_mp();

    return;
}                               // end unwield_item()

// This does *not* call ev_mod!
void unwear_armour(char unw)
{
    you.redraw_armour_class = 1;
    you.redraw_evasion = 1;

    switch (get_armour_ego_type( you.inv[unw] ))
    {
    case SPARM_RUNNING:
        mpr("You feel rather sluggish.");
        break;

    case SPARM_FIRE_RESISTANCE:
        mpr("\"Was it this warm in here before?\"");
        break;

    case SPARM_COLD_RESISTANCE:
        mpr("You catch a bit of a chill.");
        break;

    case SPARM_POISON_RESISTANCE:
        if (!player_res_poison())
            mpr("You feel less healthy.");
        break;

    case SPARM_SEE_INVISIBLE:
        if (!player_see_invis())
            mpr("You feel less perceptive.");
        break;

    case SPARM_DARKNESS:        // I do not understand this {dlb}
        if (you.duration[DUR_INVIS])
            you.duration[DUR_INVIS] = 1;
        break;

    case SPARM_STRENGTH:
        modify_stat(STAT_STRENGTH, -3, false);
        break;

    case SPARM_DEXTERITY:
        modify_stat(STAT_DEXTERITY, -3, false);
        break;

    case SPARM_INTELLIGENCE:
        modify_stat(STAT_INTELLIGENCE, -3, false);
        break;

    case SPARM_PONDEROUSNESS:
        mpr("That put a bit of spring back into your step.");
        // you.speed -= 2;
        break;

    case SPARM_LEVITATION:
        //you.duration[DUR_LEVITATION]++;
        if (you.duration[DUR_LEVITATION])
            you.duration[DUR_LEVITATION] = 1;
        break;

    case SPARM_MAGIC_RESISTANCE:
        mpr("You feel less resistant to magic.");
        break;

    case SPARM_PROTECTION:
        mpr("You feel less protected.");
        break;

    case SPARM_STEALTH:
        mpr("You feel less stealthy.");
        break;

    case SPARM_RESISTANCE:
        mpr("You feel hot and cold all over.");
        break;

    case SPARM_POSITIVE_ENERGY:
        mpr("You feel vulnerable.");
        break;

    case SPARM_ARCHMAGI:
        mpr("You feel strangely numb.");
        break;

    default:
        break;
    }

    if (is_random_artefact( you.inv[unw] ))
        unuse_randart(unw);

    return;
}                               // end unwear_armour()

void unuse_randart(unsigned char unw)
{
    unuse_randart( you.inv[unw] );
}

void unuse_randart(const item_def &item)
{
    ASSERT( is_random_artefact( item ) );

    const bool ident = fully_identified(item);

    randart_properties_t proprt;
    randart_wpn_properties( item, proprt );

    if (proprt[RAP_AC])
    {
        you.redraw_armour_class = 1;
        if (!ident)
        {
            mprf("You feel less %s.",
            proprt[RAP_AC] > 0? "well-protected" : "vulnerable");
        }
    }

    if (proprt[RAP_EVASION])
    {
        you.redraw_evasion = 1;
        if (!ident)
        {
            mprf("You feel less %s.",
                 proprt[RAP_EVASION] > 0? "nimble" : "awkward");
        }
    }

    // modify ability scores, always output messages
    modify_stat( STAT_STRENGTH,     -proprt[RAP_STRENGTH],     false );
    modify_stat( STAT_INTELLIGENCE, -proprt[RAP_INTELLIGENCE], false );
    modify_stat( STAT_DEXTERITY,    -proprt[RAP_DEXTERITY],    false );

    if (proprt[RAP_NOISES] != 0)
        you.special_wield = SPWLD_NONE;
}                               // end unuse_randart()
