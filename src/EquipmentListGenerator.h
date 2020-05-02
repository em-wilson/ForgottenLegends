#ifndef FORGOTTENLEGENDS_EQUIPMENTLISTGENERATOR_H
#define FORGOTTENLEGENDS_EQUIPMENTLISTGENERATOR_H

#include <vector>


class EquipmentListGenerator {
private:
    static bool _DirectoryExists(const char *pathName);
    void build_json_files(const char *objlist_dir);
    static std::string itemJson(OBJ_INDEX_DATA *obj);
    void storeItemCategory(const char *objListDir, const std::string& fileName, std::vector<OBJ_INDEX_DATA *> itemList);
public:
    static void Generate();
};


#endif //FORGOTTENLEGENDS_EQUIPMENTLISTGENERATOR_H
