/*
TODO: Fix the spiders
*/
/*
* Copyright (C) 2005 - 2012 MaNGOS <http://www.getmangos.org/>
*
* Copyright (C) 2008 - 2012 TrinityCore <http://www.trinitycore.org/>
*
* Copyright (C) 2011 - 2012 Naios <https://github.com/Naios>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "firelands.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "PassiveAI.h"
#include "SpellScript.h"
#include "MoveSplineInit.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Creature.h"

enum Phase
{
	PHASE_NON					= 1,
	PHASE_BETHILAC_UPPER		= 2,
};

enum Npc
{
	NPC_CINDERWEB_SPINNER		= 53642,
};

enum Yells
{
	SAY_AGGRO                                    = -1999971,
	SAY_SOFT_ENRAGE								 = -1999972, //TODO Add Sound
	SAY_ON_DOGS_FALL							 = -1999973, //TODO Add Sound
	SAY_ON_DEAD									 = -1999974, //TODO Add Sound
};

enum Spells
{
	// Bethilac
	// Phase 1
	SPELL_EMBER_FLARE = 98934, // And Phase 2
	SPELL_METEOR_BURN = 99076,
	SPELL_CONSUME = 99304, // And Cinderweb Drone and Phase 2
	SPELL_SMOLDERING_DEVASTATION = 99052,

	// Phase 2
	SPELL_FRENZY = 23537,
	SPELL_THE_WIDOWS_KISS = 99506,

	// Ciderweb Spinner
	SPELL_BURNING_ACID = 98471, // And Cinderweb Drone
	SPELL_FIERY_WEB_SPIN_H = 97202,

	// Cinderweb Drone
	SPELL_BOILING_SPLATTER = 0, // ID ?
	SPELL_FIXATE_H = 49026,

	//Cinderweb Spiderling
	SPELL_SEEPING_VENOM = 97079,

	// Engorged Broodling
	SPELL_VOLATILE_BURST_H = 99990,
};

enum Events
{
	EVENT_SUMMON_CINDERWEB_SPINNER = 1,
	EVENT_SPINNER_BURNING_ACID = 2,
};

Position const CinderwebSummonPos[7] =
{
	{55.614f, 385.11f, 0, 0},
	{61.906f, 352.12f, 0, 0},
	{49.118f, 352.12f, 0, 0},
	{36.080f, 357.46f, 0, 0},
	{28.873f, 372.63f, 0, 0},
	{32.848f, 382.93f, 0, 0},
	{39.499f, 393.54f, 0, 0}
};

// Grounds
const float groundLow = 74.042f;
const float groundUp = 111.767f;

// Event Timers
const int timerSummonCinderwebSpinner = 11000;
const int timerSpinnerBurningAcid = 7000;

/**** Beth'ilac ****/

class boss_bethtilac : public CreatureScript
{
public:
	boss_bethtilac() : CreatureScript("boss_bethtilac"){}

	CreatureAI* GetAI(Creature* creature) const
	{
		return new boss_bethtilacAI(creature);
	}

	struct boss_bethtilacAI : public BossAI
	{
		boss_bethtilacAI(Creature* c) : BossAI(c, DATA_BETHILAC)
		{
			instance = me->GetInstanceScript();

			me->setPowerType(POWER_FOCUS);
			me->SetMaxPower(POWER_FOCUS,1000);

			Reset();
		}

		InstanceScript* instance;
		Phase phase;
		CreatureIds npc;

		void Reset()
		{
			events.Reset();

			instance->SetBossState(DATA_BETHILAC, NOT_STARTED);

			me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
			me->GetMotionMaster()->MoveTargetedHome();

			phase = PHASE_NON;

			_Reset();
		}

		void JustSummoned(Creature* summon)
		{
			summons.Summon(summon);
			summon->setActive(true);
			summon->AI()->DoZoneInCombat();
		}

		void KilledUnit(Unit * /*victim*/)
		{
		}

		void JustReachedHome()
		{
			instance->SetBossState(DATA_BETHILAC, FAIL);
			summons.DespawnAll();
		}

		void JustDied(Unit * /*victim*/)
		{
			summons.DespawnAll();
			instance->SetBossState(DATA_BETHILAC, DONE);
			DoPlaySoundToSet(me, SAY_ON_DEAD);

			_JustDied();
		}

		void EnterCombat(Unit* who)
		{
			phase = PHASE_BETHILAC_UPPER;
			instance->SetBossState(DATA_BETHILAC, IN_PROGRESS);

			events.ScheduleEvent(EVENT_SUMMON_CINDERWEB_SPINNER, timerSummonCinderwebSpinner);

			me->GetMotionMaster()->MovePoint(0, me->GetPositionX(),me->GetPositionY(), groundUp);
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

			_EnterCombat();
		}

		void UpdateAI(uint32 diff)
		{
			if (!me->getVictim()) {}

			events.Update(diff);

			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			while (uint32 eventId = events.ExecuteEvent())
			{
				switch (eventId)
				{

					if(phase == PHASE_BETHILAC_UPPER)
					{
						case EVENT_SUMMON_CINDERWEB_SPINNER:

							for(int i=1; i<8; i++)
							{
							me->SummonCreature(NPC_CINDERWEB_SPINNER,CinderwebSummonPos[i].GetPositionX()
								,CinderwebSummonPos[i].GetPositionY(),groundUp,TEMPSUMMON_CORPSE_DESPAWN);
							}

							events.ScheduleEvent(EVENT_SUMMON_CINDERWEB_SPINNER, timerSummonCinderwebSpinner);
						break;

					}else
					{



					}

					if (!UpdateVictim())
						return;

					DoMeleeAttackIfReady();
				}
			}
		}
	};
};
/*
/**** Cinderweb Spinner ****/

class npc_cinderweb_spinner : public CreatureScript
{
	public:
		npc_cinderweb_spinner() : CreatureScript("npc_cinderweb_spinner") { }
	
	CreatureAI* GetAI(Creature* creature) const
		{
				return new npc_cinderweb_spinnerAI(creature);
		}

	struct npc_cinderweb_spinnerAI : public ScriptedAI //Scripted_NoMovementAI
	{
		npc_cinderweb_spinnerAI(Creature* c) : ScriptedAI(c) //,
                //instance(creature->GetInstanceScript())
		{
			//InstanceScript* instance;
			//EventMap events;			
			instance = me->GetInstanceScript();

			Reset();
		}

		InstanceScript* instance;
		EventMap events;

		void JustDied(Unit * /*victim*/)
		{
		}

		void Reset()
		{
			events.Reset();
		}

		void EnterCombat(Unit * /*who*/)
		{
			events.ScheduleEvent(EVENT_SPINNER_BURNING_ACID, urand(5000,10000));

			me->GetMotionMaster()->MovePoint(0, me->GetPositionX(),me->GetPositionY(), groundLow);

			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
		}

		void UpdateAI(uint32 diff)
		{
			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			events.Update(diff);

			while (uint32 eventId = events.ExecuteEvent())
			{
				switch (eventId)
				{

				case EVENT_SPINNER_BURNING_ACID:

					DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 500, true), SPELL_BURNING_ACID);

					events.ScheduleEvent(EVENT_SPINNER_BURNING_ACID, timerSpinnerBurningAcid);
					break;

				default:
					break;
				}
			}	
		}
	}; 
};

void AddSC_boss_bethtilac()
{
	new boss_bethtilac();
	new npc_cinderweb_spinner();
}