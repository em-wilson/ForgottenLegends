#include <algorithm>
#include <map>
#include <cctype>
#include <fstream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <vector>
#include "merc.h"
#include "EquipmentListGenerator.h"
#include "tables.h"

char *flag_string               args ( ( const struct flag_type *flag_table,
                                               int bits ) );

bool EquipmentListGenerator::_DirectoryExists(const char *pathName) {
    struct stat info;

    if( stat( pathName, &info ) != 0 )
        return false;
    else if( info.st_mode & S_IFDIR )  // S_ISDIR() doesn't exist on my windows
        return true;
    else
        return false;
}

void EquipmentListGenerator::Generate() {
    char log_buf[100];
    char objlist_dir[100];
    snprintf(objlist_dir, sizeof(objlist_dir), "%s%s", DATA_DIR, OBJLIST_DIR);
    if ( !EquipmentListGenerator::_DirectoryExists( objlist_dir ) ) {
        snprintf(log_buf, sizeof(log_buf), "Can't generate equipment list. Directory '%s' does not exists.", objlist_dir);
        log_string(log_buf);
        return;
    }

    auto generator = new EquipmentListGenerator();
    generator->build_json_files(objlist_dir);
}

std::string get_weapon_type(int weapon_value) {
    switch (weapon_value ) {
        case WEAPON_AXE:
            return "axe";
            break;
        case WEAPON_DAGGER:
            return "dagger";
            break;
        case WEAPON_EXOTIC:
            return "exotic";
            break;
        case WEAPON_FLAIL:
            return "flail";
            break;
        case WEAPON_MACE:
            return "mace";
            break;
        case WEAPON_POLEARM:
            return "polearm";
            break;
        case WEAPON_SPEAR:
            return "spear";
            break;
        case WEAPON_SWORD:
            return "sword";
            break;
        case WEAPON_WHIP:
            return "whip";
            break;
    }
    return "";
}

std::vector<std::string> get_armour_types(long wear_flags) {
    std::vector<std::string> positions;
    if ( IS_SET(wear_flags, ITEM_WEAR_FINGER)) {
        positions.emplace_back("finger");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_NECK)) {
        positions.emplace_back("neck");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_BACK)) {
        positions.emplace_back("back");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_BODY)) {
        positions.emplace_back("body");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_HEAD)) {
        positions.emplace_back("head");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_LEGS)) {
        positions.emplace_back("legs");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_FEET)) {
        positions.emplace_back("feet");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_HANDS)) {
        positions.emplace_back("hands");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_ARMS)) {
        positions.emplace_back("arms");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_SHIELD)) {
        positions.emplace_back("shield");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_FLOAT)) {
        positions.emplace_back("float");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_ABOUT)) {
        positions.emplace_back("about");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_WAIST)) {
        positions.emplace_back("waist");
    }
    if ( IS_SET(wear_flags, ITEM_WEAR_WRIST)) {
        positions.emplace_back("wrist");
    }
    return positions;
}

void EquipmentListGenerator::build_json_files(const char *objlist_dir) {
    char log_buf[100];
    log_string( "Generating equipment list..." );

    std::map<std::string, std::vector<OBJ_INDEX_DATA*>> itemDict;

    // Build the dictionary
    for (auto & i : obj_index_hash) {
        for ( auto obj = i; obj != nullptr; obj = obj->next ) {
            std::vector<std::string> item_categories;
            std::string item_category;

            // Needs to be player-equippable
            if ( obj->level < 0 || obj->level > LEVEL_HERO ) {
                continue;
            }

            if ( IS_SET(obj->extra_flags, ITEM_NOSHOW) ) {
                continue;
            }

            if ( !str_cmp(obj->name, "super weapon") ) {
                continue;
            }

            switch ( obj->item_type ) {
                case ITEM_WEAPON:
                    item_category = get_weapon_type(obj->value[0]);
                    if ( !item_category.empty() ) {
                        item_categories.push_back(item_category);
                    }
                    break;

                case ITEM_BOW:
                    item_categories.emplace_back("bow");
                    break;

                case ITEM_GEM:
                case ITEM_JEWELRY:
                case ITEM_CONTAINER:
                case ITEM_TREASURE:
                case ITEM_ARMOR:
                    std::vector<std::string> pos_list = get_armour_types( obj->wear_flags );
                    item_categories.insert(item_categories.end(), pos_list.begin(), pos_list.end());
                    break;
            }

            for (auto & item_categorie : item_categories) {
                itemDict[item_categorie].push_back(obj);
            }
        }
    }

    // Store it out
    for(auto & iter : itemDict)
    {
        std::string fileName = iter.first;
        std::vector<OBJ_INDEX_DATA*> itemList = iter.second;
        this->storeItemCategory(objlist_dir, fileName, itemList);
    }

    snprintf(log_buf, sizeof(log_buf), "Stored %lu categories of items.", itemDict.size());
    log_string(log_buf);
    log_string("Done generating equipment list." );
}

void EquipmentListGenerator::storeItemCategory(const char *objListDir, const std::string& fileName,
                                               std::vector<OBJ_INDEX_DATA *> itemList) {
    std::stringstream outpath;
    outpath << objListDir << fileName << ".json";
    std::ofstream outfile (outpath.str());

    outfile << "[";

    for (auto iter = itemList.begin(); iter != itemList.end(); ++iter) {
        if ( iter != itemList.begin() ) {
            outfile << "," << std::endl;
        }
        outfile << this->itemJson(*iter);
    }

    outfile << "]" << std::endl;

    outfile.close();

}

std::string replace_quotes(const char *input) {
    std::stringstream s;
    int nLen = strlen(input);
    for (auto i = 0; i <= nLen; i++)
    {
        if ( input[i] == '"' ) {
            s << '\\';
            s << '\"';
        } else if ( input[i] ) {
            s << input[i];
        }
    }
    return s.str();
}

std::string EquipmentListGenerator::itemJson(OBJ_INDEX_DATA *obj) {
    std::ostringstream json;

    std::string name = replace_quotes(obj->short_descr);

    json << "{" << std::endl;
    json << R"(    "id": ")" << obj->vnum << "\"," << std::endl;
    json << R"(    "name": ")" << name << "\"," << std::endl;
    json << R"(    "area": ")" << obj->area->name << "\"," << std::endl;
    json << R"(    "level": ")" << obj->level << "\"," << std::endl;
    json << R"(    "type": ")" << item_name(obj->item_type) << "\"," << std::endl;

    int avgDam;
    std::string bit_name;
    switch ( obj->item_type ) {
        default:
            char buf[100];
            snprintf(buf, sizeof(buf), "%d/%d/%d/%d",
                     obj->value[0],
                     obj->value[1],
                     obj->value[2],
                     obj->value[3] );

            if (!str_cmp(buf, "0/0/0/0" )) {
                json << "    \"stats\": null," << std::endl;
            } else {
                json << R"(    "stats": ")" << buf << "\"," << std::endl;
            }
            break;

        case ITEM_WEAPON:
            bit_name = weapon_bit_name( obj->value[4]);
            avgDam = (1 + obj->value[2]) * obj->value[1] / 2;
            json << "    \"range\": null," << std::endl;
            json << R"(    "damage": ")" << obj->value[1] << "d" << obj->value[2] << " (" << avgDam << ")\"," << std::endl;
            if ( !bit_name.empty() ) {
                json << R"(    "weapon_bits": ")" << bit_name << "\"," << std::endl;
            } else {
                json << "    \"weapon_bits\": null," << std::endl;
            }
            break;

        case ITEM_BOW:
            avgDam = (1 + obj->value[2]) * obj->value[1] / 2;
            json << R"(    "range": ")" << obj->value[0] << "\"," << std::endl;
            json << R"(    "damage": ")" << obj->value[1] << "d" << obj->value[2] << " (" << avgDam << ")\"," << std::endl;
            json << "    \"weapon_bits\": null," << std::endl;
            break;

        case ITEM_CONTAINER:
            json << R"(    "capacity": ")" << obj->value[3] << "\"," << std::endl;
            json << R"(    "weight_mult": ")" << obj->value[4] << "\"," << std::endl;
            break;

        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL:
            json << R"(    "spell_level": ")" << (obj->value[0] > 0 ? obj->value[0] : 0) << "\"," << std::endl;
            json << R"(    "spell_1": ")" << (obj->value[1] != -1 ? skill_table[obj->value[1]].name : "none") << "\"," << std::endl;
            json << R"(    "spell_2": ")" << (obj->value[2] != -1 ? skill_table[obj->value[2]].name : "none") << "\"," << std::endl;
            json << R"(    "spell_3": ")" << (obj->value[3] != -1 ? skill_table[obj->value[3]].name : "none") << "\"," << std::endl;
            json << R"(    "spell_4": ")" << (obj->value[4] != -1 ? skill_table[obj->value[4]].name : "none") << "\"," << std::endl;
            break;

        case ITEM_STAFF:
        case ITEM_WAND:
            json << R"(    "spell_level": ")" << obj->value[0] << "\"," << std::endl;
            json << R"(    "charges_total": ")" << obj->value[1] << "\"," << std::endl;
            json << R"(    "charges_remaining": ")" << obj->value[2] << "\"," << std::endl;
            json << R"(    "spell": ")" << (obj->value[3] != -1 ? skill_table[obj->value[3]].name : "none") << "\"," << std::endl;
            break;
    }

    std::vector<std::string> modifiers;
    for ( auto paf = obj->affected; paf; paf = paf->next )
    {
        std::ostringstream modifier;
        modifier << (paf->modifier > 0 ? "+" : "") << paf->modifier << flag_string( apply_flags, paf->location );
        if ( !modifier.str().empty() ) {
            modifiers.push_back(modifier.str());
        }
    }

    json << "    \"modifier\": [";
    for ( auto it = modifiers.begin(); it != modifiers.end(); ++it ) {
        if ( it != modifiers.begin() ) {
            json << ",";
        }
        json << "\"" << *it << "\"";
    }
    json << "]";
    std::string extra_bit;
    extra_bit = extra_bit_name( obj->extra_flags );
    if ( !extra_bit.empty() ) {
        json << "," << std::endl << "    \"extra\": \"" << extra_bit << "\"";
    }
    json << "}";
    return json.str();
}
