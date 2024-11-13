//Other than gathering resources,
//    SCVs must
//
//            Retreat from dangerous
//            situations(e.g.,
//                       enemy rushes or harass) to avoid resource loss deaths.
//
//        Repair damaged Battlecruisers during
//        or after engagements
//               .
//
//           Repair structures during enemy attacks to maintain defenses.
//
//           Attack in some urgent situations
//            (e.g., when the enemy is attacking the main base).


#include "BasicSc2Bot.h"

using namespace sc2;

// Main function to control SCVs
void BasicSc2Bot::ControlSCVs() {
	SCVScout();
    RetreatFromDanger();
    RepairUnits();
    RepairStructures();
    SCVAttackEmergency();
}

// SCVs scout the map to find enemy bases
void BasicSc2Bot::SCVScout() {
    const sc2::ObservationInterface* observation = Observation();

    // Check if we have enough SCVs
    sc2::Units scvs = observation->GetUnits(sc2::Unit::Alliance::Self, sc2::IsUnit(sc2::UNIT_TYPEID::TERRAN_SCV));
    if (scvs.size() < 5 || scout_complete) {
        return;
    }

    if (is_scouting) {
        // Get scouting SCV
        scv_scout = observation->GetUnit(scv_scout->tag);

        if (scv_scout) {
            // Update the scouting SCV's current locationcd.
            scout_location = scv_scout->pos;

            // Check if SCV has reached the current target location
            float distance_to_target = sc2::Distance2D(scout_location, enemy_start_locations[current_scout_location_index]);
            if (distance_to_target < 10.0f) {
                // Check for enemy town halls
                sc2::Units enemy_structures = observation->GetUnits(sc2::Unit::Alliance::Enemy, [](const sc2::Unit& unit) {
                    return unit.unit_type == sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER ||
                        unit.unit_type == sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND ||
                        unit.unit_type == sc2::UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING ||
                        unit.unit_type == sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING ||
                        unit.unit_type == sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS ||
                        unit.unit_type == sc2::UNIT_TYPEID::ZERG_HATCHERY ||
                        unit.unit_type == sc2::UNIT_TYPEID::ZERG_LAIR ||
                        unit.unit_type == sc2::UNIT_TYPEID::ZERG_HIVE ||
                        unit.unit_type == sc2::UNIT_TYPEID::PROTOSS_NEXUS;
                    });

                for (const auto& structure : enemy_structures) {
                    if (sc2::Distance2D(scout_location, structure->pos) < 10.0f) {
                        // Set the enemy start location and stop scouting
                        enemy_start_location = structure->pos;
                        scv_scout = nullptr;
                        is_scouting = false;
                        scout_complete = true;
                        return;
                    }
                }

                // Move to the next potential enemy location if no town hall is found here
                current_scout_location_index++;
                // Scout to the next location
                Actions()->UnitCommand(scv_scout, sc2::ABILITY_ID::MOVE_MOVE, enemy_start_locations[current_scout_location_index]);
         
            }
        }
    }
    else {
        // Assign an SCV to scout when no SCVs are scouting
        for (const auto& scv : scvs) {
            if (scv->orders.empty()) {
                scv_scout = scv;
                is_scouting = true;
                current_scout_location_index = 0;  // Start from the first location

                // Set the initial position of the scouting SCV
                scout_location = scv->pos;

                // Command SCV to move to the initial possible enemy location
                Actions()->UnitCommand(scv_scout, sc2::ABILITY_ID::MOVE_MOVE, enemy_start_locations[current_scout_location_index]);
                break;
            }
        }
    }
}

// SCVs repair damaged Battlecruisers during or after engagements
void BasicSc2Bot::RepairUnits() {
    for (const auto &unit : Observation()->GetUnits(Unit::Alliance::Self)) {
        if (unit->unit_type == UNIT_TYPEID::TERRAN_SCV) {
            const Unit *target = FindDamagedUnit();
            if (target) {
                Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_REPAIR, target);
            }
        }
    }
}

// SCVs repair damaged structures during enemy attacks
void BasicSc2Bot::RepairStructures() {
    for (const auto &unit : Observation()->GetUnits(Unit::Alliance::Self)) {
        if (unit->unit_type == UNIT_TYPEID::TERRAN_SCV) {
            const Unit *target = FindDamagedStructure();
            if (target) {
                Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_REPAIR, target);
            }
        }
    }
}

// SCVs attack in urgent situations (e.g., enemy attacking the main base)
void BasicSc2Bot::SCVAttackEmergency() {
    if (IsMainBaseUnderAttack()) {
        for (const auto &unit : Observation()->GetUnits(Unit::Alliance::Self)) {
            if (unit->unit_type == UNIT_TYPEID::TERRAN_SCV) {
                const Unit *target = FindClosestEnemy(unit->pos);
                if (target) {
                    Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, target);
                }
            }
        }
    }
}

