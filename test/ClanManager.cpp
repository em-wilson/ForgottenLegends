#include <catch2/catch_test_macros.hpp>
#include "merc.h"
#include "clans/ClanManager.h"
#include "clans/ClanReader.h"
#include "clans/ClanWriter.h"

using std::vector;

class FakeClanReader : public IClanReader {
    public:
        std::list<Clan *> loadClans() {
            return std::list<Clan *>();
        }
};

class FakeClanWriter : public IClanWriter {
    public:
        FakeClanWriter() {
            this->clear();
        }

        void clear() {
            written = false;
            clans_saved = 0;
            clans_deleted = 0;
        }

        void write_clan_list(vector<Clan *> clan_list) {
            written = true;
        }

        void save_clan(Clan * clan) {
            clans_saved++;
        }

        void delete_clan(Clan *clan) {
            clans_deleted++;
        }

        bool written;
        int clans_saved;
        int clans_deleted;
};

SCENARIO( "Clan Management" ) {
    GIVEN( "A clan exists" ) {
        FakeClanReader *reader = new FakeClanReader();
        FakeClanWriter *writer = new FakeClanWriter();
        ClanManager *target = new ClanManager(reader, writer);
        Clan * first_clan = new Clan();
        first_clan->setName("bar");
        target->add_clan(first_clan);

        WHEN("Looking for a non-existing clan") {
            Clan *result = target->get_clan("foo");

            THEN("The result should be NULL") {
                REQUIRE( result == NULL );
            }
        }

        WHEN("Looking for an existing clan") {
            Clan *result = target->get_clan("bar");

            THEN("The result should not be NULL") {
                REQUIRE( result != NULL );
            }
        }

        WHEN("Writing the clan list") {
            THEN("The writer was called") {
                target->write_clan_list();
                REQUIRE(writer->written == true);
            }
        }

        WHEN("Saving a clan") {
            THEN("The writer was called") {
                target->save_clan(first_clan);
                target->save_clan(first_clan);
                REQUIRE(writer->clans_saved == 2);
            }
            writer->clans_saved = 0;
        }

        WHEN("Incrementing play time") {
            Clan *clan = target->get_clan("bar");
            REQUIRE(clan != NULL);
            REQUIRE(clan->getPlayTime() == 0);

            THEN("The play time should not decrease") {
                target->add_playtime(clan, 50);
                REQUIRE(clan->getPlayTime() == 50);
                target->add_playtime(clan, 25);
                REQUIRE(clan->getPlayTime() == 75);
            }
        }

        WHEN("Listing clan net worths") {
            Clan *clan = target->get_clan("bar");
            clan->setMemberCount(5);

            auto list = target->list_clan_nw();
            REQUIRE(clan != NULL);
            
            THEN("All clans should be returned") {
                REQUIRE(list.size() == 1);
                REQUIRE(list.count("foo") == 0);
                REQUIRE(list.count("bar") == 1);
                REQUIRE(list.at("bar") == 1250);
            }
            clan->setMemberCount(0);
        }

        WHEN("A clan is added") {
            Clan *demo_clan = new Clan();
            target->add_clan(demo_clan);

            THEN("There will be two clans") {
                REQUIRE(target->get_all_clans().size() == 2);
                REQUIRE(writer->clans_deleted == 0);
                REQUIRE(writer->clans_saved == 0);
            }

            THEN("One of the clans may be deleted") {
                demo_clan = target->delete_clan(demo_clan);
                REQUIRE(target->get_all_clans().size() == 1);
                REQUIRE(writer->clans_deleted == 1);
            }

            delete demo_clan;
            demo_clan = NULL;
            writer->clear();
        }

        delete first_clan;
        first_clan = NULL;
    }
}
