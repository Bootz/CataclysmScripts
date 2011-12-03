/*
* Copyright (C) 2005 - 2011 MaNGOS <http://www.getmangos.org/>
*
* Copyright (C) 2008 - 2011 TrinityCore <http://www.trinitycore.org/>
*
* Copyright (C) 2011 TrilliumEMU <http://www.arkania.net/>
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

/*###########################
#      Developer Info:      #
#   Script Coded by Naios   #
#                           #
#   Script Complete: 72%    #
#  Works with Trillium EMU  #
#     & Strawberry Core     #
###########################*/

/* TODO:
- [100% DONE] Arcing Slash (Works Perfect)
- [100% DONE] Insert Heroic Spells
- [25%] Insert the Sound data
- [100% DONE] The Script Crashes sometimes (I'm not sure weather this
is the fault of this Script or the Core)
- [100% DONE] Add Trigger for the 'Seperation Anxietly' Spell
- [25%] Add Trigger for the Fire Nova when the Spear is reaching the Ground
- [75%] Add Action that Shannox throws the Spear to Riplimb (Location Correct?)
- [75%] Add Action that Riplimb is taking the Spear back to Shannox
- [75% DONE] Add Kristall Trap Actions (Stun for Players Missing... Opcode Problem?)
- [75%] Add Fire Trap Actions (Damage of the Trap is missing)
*/

#include "ScriptPCH.h"
#include "firelands.h"

enum Yells
{
	SAY_AGGRO                                    = -1999971,
	SAY_SOFT_ENRAGE								 = -1999972, //TODO Add Sound
	SAY_ON_DOGS_FALL							 = -1999973, //TODO Add Sound
	SAY_ON_DEAD									 = -1999974, //TODO Add Sound
};

enum Spells
{

	//Shannox
	SPELL_ARCTIC_SLASH_10N = 99931,
	SPELL_ARCTIC_SLASH_25N = 101201,
	SPELL_ARCTIC_SLASH_10H = 101202,
	SPELL_ARCTIC_SLASH_25H = 101203,

	SPELL_BERSERK = 26662,

	SPELL_CALL_SPEAR = 100663,
	SPELL_HURL_SPEAR = 100002, // Dummy Effect & Damage
	SPELL_HURL_SPEAR_SUMMON = 99978, //Summons Spear of Shannox
	SPELL_HURL_SPEAR_DUMMY_SCRIPT = 100031,
	SPELL_MAGMA_RUPTURE_SHANNOX = 99840,

	SPELL_FRENZY_SHANNOX = 23537,
	SPELL_IMMOLATION_TRAP = 52606,

	// Riplimb
	SPELL_LIMB_RIP = 99832,
	SPELL_DOGGED_DETERMINATION = 101111,

	// Rageface
	SPELL_FACE_RAGE = 99947,

	// Both Dogs
	SPELL_FRENZIED_DEVOLUTION = 100064,
	SPELL_FEEDING_FRENZY_H = 100655,

	SPELL_WARY_10N = 100167, // Buff when the Dog gos in a Trap
	SPELL_WARY_25N = 101215,
	SPELL_WARY_10H = 101216,
	SPELL_WARY_25H = 101217,

	// Misc
	SPELL_SEPERATION_ANXIETY = 99835,

	//Spear Abilities
	SPELL_MAGMA_FLARE = 100495, // Inflicts Fire damage to enemies within 50 yards.
	SPELL_MAGMA_RUPTURE = 100003, // Calls forth magma eruptions to damage nearby foes. (Dummy Effect)
	SPELL_MAGMA_RUPTURE_VISUAL = 99841,

	//Traps Abilities
	CRYSTAL_PRISON_EFFECT = 99837,

	// Dont know weather i implement that...
	SPELL_FRENZY_RIPLIMB = 100522, //Increases attack speed by 30% and physical damage dealt by 30%.

};

enum Events
{
	//Shannox
	EVENT_IMMOLTATION_TRAP = 1, // Every 10s
	EVENT_BERSERK = 2, // After 10m
	EVENT_ARCING_SLASH = 3, // Every 12s
	EVENT_HURL_SPEAR_OR_MAGMA_RUPTUTRE = 4, // Every 42s
	EVENT_SUMMON_SPEAR = 5, // After EVENT_HURL_SPEAR_OR_MAGMA_RUPTUTRE
	EVENT_SUMMON_CRYSTAL_PRISON = 6, // Every 25s

	//Riplimb
	EVENT_LIMB_RIP = 7, // i Dont know...
	EVENT_RIPLIMB_RESPAWN_H = 8,
	EVENT_TAKING_SPEAR_DELAY = 9,

	//Rageface
	EVENT_FACE_RAGE = 10,

	// Trigger for the Crystal Trap
	EVENT_CRYSTAL_TRAP_TRIGGER = 11,

	// Trigger for self Dispawn (Crystal Prison)
	EVENT_CRYSTAL_PRISON_DESPAWN = 12,

};

/*#########################
######### Shannox #########
#########################*/

class boss_shannox : public CreatureScript
{
public:
	boss_shannox() : CreatureScript("boss_shannox"){}

	CreatureAI* GetAI(Creature* creature) const
	{
		return new boss_shannoxAI(creature);
	}

	struct boss_shannoxAI : public BossAI
	{
		boss_shannoxAI(Creature* c) : BossAI(c, DATA_SHANNOX)
		{
			instance = me->GetInstanceScript();
			// TODO Add not Tauntable Flag

			pRiplimb = NULL;
			pRageface = NULL;

			softEnrage = false;
			riplimbIsRespawning = false;

			Reset();
		}

		InstanceScript* instance;
		Creature* pRiplimb;
		Creature* pRageface;
		bool softEnrage;
		bool riplimbIsRespawning;
		bool bucketListCheckPoints[5];
		bool hurlSpeer;

		void Reset()
		{
			me->RemoveAllAuras();
			me->GetMotionMaster()->MovePath(PATH_SHANNOX,true);
			softEnrage = false;
			riplimbIsRespawning = false;
			hurlSpeer = false;
			events.Reset();

			if(pRiplimb != NULL)  // Prevents Crashes
			{
				if (pRiplimb->isDead())
					pRiplimb -> Respawn();
			}else
			{
				pRiplimb = me->SummonCreature(NPC_RIPLIMB, me->GetPositionX()-5
					,me->GetPositionY()-5,me->GetPositionZ(),TEMPSUMMON_MANUAL_DESPAWN);
			};

			if(pRageface != NULL)  // Prevents Crashes
			{
				if (pRageface->isDead())
					pRageface -> Respawn();
			}else
			{
				pRageface = me->SummonCreature(NPC_RAGEFACE, me->GetPositionX()+5
					,me->GetPositionY()+5,me->GetPositionZ(),TEMPSUMMON_MANUAL_DESPAWN);
			};

			//me->SetReactState(REACT_PASSIVE); //TODO Only for testing

			//me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);  //TODO Only for testing

			_Reset();
		}

		void JustSummoned(Creature* summon)
		{
			summons.Summon(summon);
			summon->setActive(true);

			if(me->isInCombat())
				summon->AI()->DoZoneInCombat();
		}

		void KilledUnit(Unit * /*victim*/)
		{
		}

		void JustDied(Unit * /*victim*/)
		{
			DoScriptText(SAY_ON_DEAD, me);

			summons.DespawnAll();

			_JustDied();
		}

		void EnterCombat(Unit* who)
		{
			DoZoneInCombat();
			me->CallForHelp(50);

			instance->SetData(DATA_CURRENT_ENCOUNTER_PHASE, PHASE_SHANNOX_HAS_SPEER);

			events.ScheduleEvent(EVENT_IMMOLTATION_TRAP, 10000);
			events.ScheduleEvent(EVENT_ARCING_SLASH, 12000);
			events.ScheduleEvent(EVENT_HURL_SPEAR_OR_MAGMA_RUPTUTRE, 20000); //TODO Find out the correct Time
			events.ScheduleEvent(EVENT_SUMMON_CRYSTAL_PRISON, 25000);
			events.ScheduleEvent(EVENT_BERSERK, 10 * MINUTE * IN_MILLISECONDS);

			DoScriptText(SAY_AGGRO, me, who);

			_EnterCombat();
		}

		void UpdateAI(const uint32 diff)
		{
			if (!me->getVictim()) {}

			events.Update(diff);

			if (me->HasUnitState(UNIT_STAT_CASTING))
				return;

			if(hurlSpeer)
			{
				me->MonsterSay("1",0,0);
				hurlSpeer = false;
				me->CastSpell(pRiplimb->GetPositionX()+(urand(0,2000)-1000),pRiplimb->GetPositionY()+(urand(0,2000)-1000),
					pRiplimb->GetPositionZ(),SPELL_HURL_SPEAR_SUMMON,true);
				instance->SetData(DATA_CURRENT_ENCOUNTER_PHASE, PHASE_SPEAR_ON_THE_GROUND);

			}

			while (uint32 eventId = events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_IMMOLTATION_TRAP:
					DoCastVictim(SPELL_IMMOLATION_TRAP,true); // Random Target or Victim?
					events.ScheduleEvent(EVENT_IMMOLTATION_TRAP, 10000);
					break;

				case EVENT_BERSERK:
					DoCast(me, SPELL_BERSERK);
					break;

				case EVENT_ARCING_SLASH:
					DoCastVictim(RAID_MODE(SPELL_ARCTIC_SLASH_10N, SPELL_ARCTIC_SLASH_25N,
						SPELL_ARCTIC_SLASH_10H, SPELL_ARCTIC_SLASH_25H));
					events.ScheduleEvent(EVENT_ARCING_SLASH, 12000);
					break;

				case EVENT_HURL_SPEAR_OR_MAGMA_RUPTUTRE:

					if(pRiplimb->isDead())
					{ // Cast Magma Rupture when Ripclimb is Death
						DoCastVictim(SPELL_MAGMA_RUPTURE_SHANNOX);
						events.ScheduleEvent(EVENT_HURL_SPEAR_OR_MAGMA_RUPTUTRE, 42000);
					}else
					{
						// Throw Spear if Riplimb is Alive and Shannox has the Spear
						if (instance->GetData(DATA_CURRENT_ENCOUNTER_PHASE) == PHASE_SHANNOX_HAS_SPEER)
						{
							me->MonsterSay("2",0,0);
							events.ScheduleEvent(EVENT_HURL_SPEAR_OR_MAGMA_RUPTUTRE, 42000);

							hurlSpeer = true;

							me->MonsterSay("3",0,0);
							DoCast(pRiplimb ,SPELL_HURL_SPEAR);
							me->MonsterSay("4",0,0);

						}else
							// Shifts the Event back if Shannox has not the Spear yet
							events.ScheduleEvent(EVENT_HURL_SPEAR_OR_MAGMA_RUPTUTRE, 10000);
					}

					break;

				case EVENT_SUMMON_SPEAR:

					me->CastSpell(pRiplimb->GetPositionX()+(urand(0,2000)-1000),pRiplimb->GetPositionY()+(urand(0,2000)-1000),
						pRiplimb->GetPositionZ(),SPELL_HURL_SPEAR_SUMMON,true);

					//DoCast(SPELL_HURL_SPEAR_DUMMY_SCRIPT);

					break;

				case EVENT_SUMMON_CRYSTAL_PRISON:
					if (Unit* tempTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 500, true))
						pRiplimb = me->SummonCreature(NPC_CRYSTAL_TRAP, tempTarget->GetPositionX()
						,tempTarget->GetPositionY(),tempTarget->GetPositionZ(),TEMPSUMMON_MANUAL_DESPAWN);
					events.ScheduleEvent(EVENT_SUMMON_CRYSTAL_PRISON, 25000);
					break;

				case EVENT_RIPLIMB_RESPAWN_H:
					riplimbIsRespawning = false;
					pRiplimb->Respawn();
					DoZoneInCombat();
					break;

				default:
					break;
				}
			}

			if (!UpdateVictim())
				return;

			if ((pRiplimb->isDead() || pRageface -> isDead()) && !softEnrage)
			{
				// Heroic: Respawn Riplimb 30s after he is Death
				if(pRiplimb->isDead() && me->GetMap()->IsHeroic() && (!riplimbIsRespawning))
				{
					riplimbIsRespawning = true;
					events.ScheduleEvent(EVENT_RIPLIMB_RESPAWN_H, 30000);
				}

				DoCast(me, SPELL_FRENZY_SHANNOX);
				me->MonsterTextEmote(SAY_ON_DOGS_FALL, 0, true);
				me->MonsterYell(SAY_SOFT_ENRAGE,0,0);
				softEnrage = true;
			}

			if((pRiplimb->GetDistance2d(me) >= 70 || pRageface->GetDistance2d(me) >= 70) && (!me->HasAura(SPELL_SEPERATION_ANXIETY)))
				DoCast(me, SPELL_SEPERATION_ANXIETY);

			if (instance->GetData(DATA_CURRENT_ENCOUNTER_PHASE) == PHASE_RIPLIMB_BRINGS_SPEER && pRiplimb -> GetDistance(me) <= 1)
			{
				instance->SetData(DATA_CURRENT_ENCOUNTER_PHASE, PHASE_SHANNOX_HAS_SPEER);
			}

			DoMeleeAttackIfReady();
		}
	};
};

/*#########################
######## Rageface #########
#########################*/

class npc_rageface : public CreatureScript
{
public:
	npc_rageface() : CreatureScript("npc_rageface"){}

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npc_ragefaceAI(creature);
	}

	struct npc_ragefaceAI : public ScriptedAI
	{
		npc_ragefaceAI(Creature *c) : ScriptedAI(c)
		{
			instance = me->GetInstanceScript();

			Reset();
		}

		InstanceScript* instance;
		EventMap events;
		Unit* shallTarget;
		bool frenzy;
		Phases phase;

		void Reset()
		{
			me->SetReactState(REACT_PASSIVE); //TODO Only for testing

			me->RemoveAllAuras();
			events.Reset();
			frenzy = false;
			shallTarget = NULL;

			//me->GetMotionMaster()->MoveTargetedHome();
			if(GetShannox() != NULL)
				me->GetMotionMaster()->MoveFollow(GetShannox(), 10, 0);
		}

		void KilledUnit(Unit * /*victim*/)
		{
		}

		void JustDied(Unit * /*victim*/)
		{
		}

		void EnterCombat(Unit * /*who*/)
		{
			DoZoneInCombat();
			me->CallForHelp(50);

			me->GetMotionMaster()->MoveChase(me->getVictim());

			//events.ScheduleEvent(EVENT_FACE_RAGE, 15000); //TODO Find out the correct Time
		}

		void SelectNewTarget()
		{
			shallTarget = SelectTarget(SELECT_TARGET_RANDOM, 1, 500, true);
			me->getThreatManager().resetAllAggro();
			me->AddThreat(shallTarget, 500.0f);
			me->Attack(shallTarget, true);
			//me->GetMotionMaster()->MoveChase(shallTarget);
		}

		void DamageTaken(Unit* attacker, uint32 damage)
		{
			//me->MonsterSay("Triggered Damage Taken",0,0);

			if (damage >= 1/* && me->HasAura(BUFF_FACE_RAGE)*/)
			{	
				me->MonsterSay("Triggered",0,0);
				me->RemoveAura(SPELL_FACE_RAGE);
				me->getVictim()->ClearUnitState(UNIT_STAT_STUNNED);
				me->SetTarget(me->Attack(SelectTarget(SELECT_TARGET_RANDOM, 1, 500, true),true));
				//events.ScheduleEvent(EVENT_FACE_RAGE, 15000); //TODO Find out the correct Time
			}
		}

		void UpdateAI(const uint32 diff)
		{
			if (!me->getVictim()) {}

			events.Update(diff);

			while (uint32 eventId = events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_FACE_RAGE:
					DoCastVictim(SPELL_FACE_RAGE);
					//me->getVictim()->SetFlag(UNIT_FIELD_FLAGS,  UNIT_STAT_STUNNED);
					//me->getVictim()->AddUnitState(UNIT_STAT_STUNNED);
					break;
				default:
					break;
				}
			}

			if(GetShannox() != NULL)
			{
				if(GetShannox()->GetHealthPct() <= 30 && frenzy == false)
				{
					frenzy = true;
					DoCast(me, SPELL_FRENZIED_DEVOLUTION);
				}

				if(GetShannox()->GetDistance2d(me) >= 70 && !me->HasAura(SPELL_SEPERATION_ANXIETY)) //TODO Sniff right Distance
				{
					DoCast(me, SPELL_SEPERATION_ANXIETY);
				}

			}

			if (!UpdateVictim())
				return;

			DoMeleeAttackIfReady();
		}

		void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/)
		{
			// Feeding Frenzy (Heroic Ability)
			if(me->GetMap()->IsHeroic() && damage > 0)
				DoCast(me, SPELL_FEEDING_FRENZY_H);
		}

		Creature* GetShannox()
		{
			return ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_SHANNOX));
		}

	};
};

/*#########################
######### Riplimb #########
#########################*/

class npc_riplimb : public CreatureScript
{
public:
	npc_riplimb() : CreatureScript("npc_riplimb"){}

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npc_riplimbAI(creature);
	}

	struct npc_riplimbAI : public ScriptedAI
	{
		npc_riplimbAI(Creature *c) : ScriptedAI(c)
		{
			instance = me->GetInstanceScript();

			Reset();
		}

		InstanceScript* instance;
		EventMap events;
		bool frenzy;
		bool movementResetNeeded;

		void Reset()
		{
			me->RemoveAllAuras();
			events.Reset();
			me->GetMotionMaster()->MoveTargetedHome();
			frenzy = false;
			movementResetNeeded = false;

			if(GetShannox() != NULL)
				me->GetMotionMaster()->MoveFollow(GetShannox(), 13, 0);

			//me->SetReactState(REACT_PASSIVE); //TODO Only for testing
		}

		void KilledUnit(Unit * /*victim*/)
		{
		}

		void JustDied(Unit * /*victim*/)
		{
		}

		void EnterCombat(Unit * who)
		{	
			DoZoneInCombat();
			me->CallForHelp(50);

			me->GetMotionMaster()->MoveChase(me->getVictim());

			events.ScheduleEvent(EVENT_LIMB_RIP, 12000); //TODO Find out the correct Time
		}

		void UpdateAI(const uint32 diff)
		{
			if (!me->getVictim()) {}

			events.Update(diff);

			while (uint32 eventId = events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_LIMB_RIP:
					DoCastVictim(SPELL_LIMB_RIP);	
					events.ScheduleEvent(EVENT_LIMB_RIP, 12000); //TODO Find out the correct Time
					break;
				case EVENT_TAKING_SPEAR_DELAY:
					me->GetMotionMaster()->MovePoint(0,GetSpear()->GetPositionX(),GetSpear()->GetPositionY(),GetSpear()->GetPositionZ());
					break;
				default:
					break;
				}
			}

			if(GetShannox() != NULL)
			{
				if(GetShannox()->GetHealthPct() <= 30 && frenzy == false)
				{
					frenzy = true;
					DoCast(me, SPELL_FRENZIED_DEVOLUTION);
				}

				if(GetShannox()->GetDistance2d(me) >= 70 && !me->HasAura(SPELL_SEPERATION_ANXIETY)) //TODO Sniff right Distance
				{
					DoCast(me, SPELL_SEPERATION_ANXIETY);
				}
				if(!me->HasAura(SPELL_WARY_10N))
				{
					if (instance->GetData(DATA_CURRENT_ENCOUNTER_PHASE) == PHASE_SPEAR_ON_THE_GROUND)
					{
						instance->SetData(DATA_CURRENT_ENCOUNTER_PHASE, PHASE_RIPLIMB_GOS_TO_SPEER);
						events.ScheduleEvent(EVENT_TAKING_SPEAR_DELAY, 3500);
					}

					if (instance->GetData(DATA_CURRENT_ENCOUNTER_PHASE) == PHASE_RIPLIMB_GOS_TO_SPEER && GetSpear()->GetDistance(me) <= 1)
					{
						instance->SetData(DATA_CURRENT_ENCOUNTER_PHASE, PHASE_RIPLIMB_BRINGS_SPEER);

						movementResetNeeded = true;
						DoCast(me, SPELL_DOGGED_DETERMINATION);
						me->GetMotionMaster()->MovePoint(0,GetShannox()->GetPositionX(),GetShannox()->GetPositionY(),GetShannox()->GetPositionZ());
					}

					if (instance->GetData(DATA_CURRENT_ENCOUNTER_PHASE) == PHASE_SHANNOX_HAS_SPEER && movementResetNeeded)
					{
						movementResetNeeded = false;
						me->MonsterSay("Should Reset",0,0);
						me->RemoveAura(SPELL_DOGGED_DETERMINATION);
						me->GetMotionMaster()->MoveChase(me->getVictim());
					}
				}
			}

			if (!UpdateVictim())
				return;

			DoMeleeAttackIfReady();
		}

		void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/)
		{
			// Feeding Frenzy (Heroic Ability)
			if(me->GetMap()->IsHeroic() && damage > 0)
				DoCast(me, SPELL_FEEDING_FRENZY_H);
		}

		Creature* GetShannox()
		{
			return ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_SHANNOX));
		}

		Creature* GetSpear()
		{
			return ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_SHANNOX_SPEAR));
		}
	};
};

/*#########################
###### Shannox Spear ######
#########################*/

class npc_shannox_spear : public CreatureScript
{
public:
	npc_shannox_spear() : CreatureScript("npc_shannox_spear"){}

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npc_shannox_spearAI(creature);
	}

	struct npc_shannox_spearAI : public ScriptedAI
	{
		npc_shannox_spearAI(Creature *c) : ScriptedAI(c)
		{
			instance = me->GetInstanceScript();

			me->SetReactState(REACT_PASSIVE);
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
		}

		InstanceScript* instance;

		void Reset()
		{
		}

		void JustDied(Unit * /*victim*/)
		{
		}

		void EnterCombat(Unit * /*who*/)
		{
			me->MonsterSay("Spear Triggered",0,0);

			/*
			if (GetRiplimb() != NULL)
			me->GetMotionMaster()->MoveJump(GetRiplimb()->GetPositionX()
			,GetRiplimb()->GetPositionY(),GetRiplimb()->GetPositionZ(),5,1);

			for(int i=0;i<10;i++)
			{
			me->CastSpell(me->GetPositionX()+(urand(0,40)-20),me->GetPositionY()+(urand(0,40)-20),
			46,SPELL_MAGMA_RUPTURE_VISUAL,true);
			}
			*/
			DoCast(SPELL_MAGMA_FLARE);
		}

		void UpdateAI(const uint32 diff)
		{
			if (!me->getVictim()) {}

			if (instance->GetData(DATA_CURRENT_ENCOUNTER_PHASE) == PHASE_SHANNOX_HAS_SPEER)
				me -> DisappearAndDie();

			if (!UpdateVictim())
				return;
		}

		Creature* GetShannox()
		{
			return ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_SHANNOX));
		}

		Creature* GetRiplimb()
		{
			return ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_RIPLIMB));
		}
	};
};

/*#########################
####### Crystal Trap ######
#########################*/

class npc_crystal_trap : public CreatureScript
{
public:
	npc_crystal_trap() : CreatureScript("npc_crystal_trap"){}

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npc_crystal_trapAI(creature);
	}

	struct npc_crystal_trapAI : public Scripted_NoMovementAI
	{
		npc_crystal_trapAI(Creature *c) : Scripted_NoMovementAI(c)
		{
			instance = me->GetInstanceScript();
			tempTarget = NULL;
			myPrison = NULL;
			//me->SetReactState(REACT_PASSIVE);
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE /*| UNIT_FLAG_NOT_SELECTABLE*/);
			events.Reset();
		}

		InstanceScript* instance;
		EventMap events;
		Unit* tempTarget;
		Creature* myPrison;

		void JustDied(Unit * /*victim*/)
		{
		}

		void Reset()
		{
			events.Reset();
		}

		void EnterCombat(Unit * /*who*/)
		{
			events.ScheduleEvent(EVENT_CRYSTAL_TRAP_TRIGGER, 4000);
			me->MonsterSay("Trap Summoned",0,0);
		}

		void UpdateAI(const uint32 diff)
		{
			if (!me->getVictim()) {}

			events.Update(diff);

			while (uint32 eventId = events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_CRYSTAL_TRAP_TRIGGER:
					// me->MonsterSay("Trap Event Triggered",0,0);

					// Riplimb has a higher Priority than Players...

					if(GetRiplimb() != NULL)
					{
						//me->MonsterSay("0",0,0);

						if(GetRiplimb()->GetDistance(me) <= 1 && (!GetRiplimb()->
							HasAura(SPELL_WARY_10N)) && GetRiplimb()->isAlive()
							&& instance->GetData(DATA_CURRENT_ENCOUNTER_PHASE) == PHASE_SHANNOX_HAS_SPEER)
						{
							//me->MonsterSay("1",0,0);
							tempTarget = GetRiplimb();

						}else
						{
							if (SelectTarget(SELECT_TARGET_NEAREST, 0, 2, true) != NULL)
							{ 
								if (SelectTarget(SELECT_TARGET_NEAREST, 0, 2, true)->GetDistance(me) <= 2)
								{ 
									//me->MonsterSay("2",0,0);
									tempTarget = SelectTarget(SELECT_TARGET_NEAREST, 0, 2, true);
								}
							}
						}
					}
					if (tempTarget == NULL) // If no Target exists try to get a new Target in 0,5s
					{
						//me->MonsterSay("3",0,0);
						events.ScheduleEvent(EVENT_CRYSTAL_TRAP_TRIGGER, 500);
					}else
					{ // Intialize Prison if tempTarget was set
						//me->MonsterSay("4",0,0);
						myPrison = me->SummonCreature(NPC_CRYSTAL_PRISON,me->GetPositionX()
							,me->GetPositionY(),me->GetPositionZ(),0, TEMPSUMMON_MANUAL_DESPAWN);
						myPrison->SetReactState(REACT_PASSIVE);
						DoCast(tempTarget,CRYSTAL_PRISON_EFFECT);
						events.ScheduleEvent(EVENT_CRYSTAL_PRISON_DESPAWN, 15000);
					}

					break;

				case EVENT_CRYSTAL_PRISON_DESPAWN:

					myPrison -> DespawnOrUnsummon();
					tempTarget -> RemoveAurasDueToSpell(CRYSTAL_PRISON_EFFECT);
					// Cast Spell Wary on Ripclimb
					if(tempTarget ->GetEntry() == NPC_RIPLIMB)
						DoCast(tempTarget,SPELL_WARY_10N, true);

					me->DisappearAndDie();

					break;

				default:
					break;
				}
			}	

			if(myPrison != NULL)
			{
				if(myPrison->isDead())
				{
					myPrison -> DespawnOrUnsummon();
					tempTarget -> RemoveAurasDueToSpell(CRYSTAL_PRISON_EFFECT);
					me->DisappearAndDie();
				}	
			}	

			if (!UpdateVictim())
				return;
		}

		Creature* GetRiplimb()
		{
			return ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_RIPLIMB));
		}
	};
};

/*#########################
####### Achievements ######
#########################*/

//Heroic Shannox (5806)
class achievement_heroic_shannox : public AchievementCriteriaScript
{
public:
	achievement_heroic_shannox() : AchievementCriteriaScript("achievement_heroic_shannox")
	{
	}

	bool OnCheck(Player* player, Unit* target)
	{
		if (!target)
			return false;

		return player->GetMap()->IsHeroic() && player->
			GetInstanceScript()->GetBossState(DATA_SHANNOX == DONE);
	}
};

//Bucket List (5829) //TODO Currently not Working!
class achievement_bucket_list : public AchievementCriteriaScript
{
public:
	achievement_bucket_list() : AchievementCriteriaScript("achievement_bucket_list")
	{
	}

	bool OnCheck(Player* /*player*/, Unit* target)
	{
		if (!target)
			return false;

		return false;
	}
};

void AddSC_boss_shannox()
{
	new boss_shannox();
	new npc_rageface();
	new npc_riplimb();
	new npc_shannox_spear();
	new npc_crystal_trap();
	new achievement_bucket_list();
	new achievement_heroic_shannox();
}
