#include "stdafx.h"
#include "DBFriendList.h"
#include "Database.h"
#include "Subsystems.h"

namespace DB {

bool DBFriendList::Create(AB::Entities::FriendList& fl)
{
    // Do nothing
    if (fl.uuid.empty() || uuids::uuid(fl.uuid).nil())
    {
        LOG_ERROR << "UUID is empty" << std::endl;
        return false;
    }
    return true;
}

bool DBFriendList::Load(AB::Entities::FriendList& fl)
{
    if (fl.uuid.empty() || uuids::uuid(fl.uuid).nil())
    {
        LOG_ERROR << "UUID is empty" << std::endl;
        return false;
    }

    Database* db = GetSubsystem<Database>();

    fl.friends.clear();
    std::ostringstream query;
    query << "SELECT * FROM `friend_list` WHERE `account_uuid` = " << db->EscapeString(fl.uuid);
    std::shared_ptr<DB::DBResult> result = db->StoreQuery(query.str());
    if (!result)
        // Maybe no friends
        return true;

    for (result = db->StoreQuery(query.str()); result; result = result->Next())
    {
        fl.friends.push_back({
            result->GetString("friend_uuid"),
            result->GetString("friend_name"),
            static_cast<AB::Entities::FriendRelation>(result->GetUInt("relation"))
        });
    }

    return true;
}

bool DBFriendList::Save(const AB::Entities::FriendList& fl)
{
    if (fl.uuid.empty() || uuids::uuid(fl.uuid).nil())
    {
        LOG_ERROR << "UUID is empty" << std::endl;
        return false;
    }

    Database* db = GetSubsystem<Database>();
    std::ostringstream query;
    DBTransaction transaction(db);
    if (!transaction.Begin())
        return false;

    // First delete all
    query << "DELETE FROM `friend_list` WHERE `account_uuid` = " << db->EscapeString(fl.uuid);

    if (!db->ExecuteQuery(query.str()))
        return false;

    if (fl.friends.size() > 0)
    {
        // Then add all
        for (const auto& f : fl.friends)
        {
            query.str("");
            query << "INSERT INTO `friend_list` (`account_uuid`, `friend_uuid`, `friend_name`, `relation`) VALUES (";
            query << db->EscapeString(fl.uuid) << ", ";
            query << db->EscapeString(f.friendName) << ", ";
            query << db->EscapeString(f.friendName) << ", ";
            query << static_cast<int>(f.relation);
            query << ")";
            if (!db->ExecuteQuery(query.str()))
                return false;
        }
    }
    // End transaction
    return transaction.Commit();
}

bool DBFriendList::Delete(const AB::Entities::FriendList& fl)
{
    // Do nothing
    if (fl.uuid.empty() || uuids::uuid(fl.uuid).nil())
    {
        LOG_ERROR << "UUID is empty" << std::endl;
        return false;
    }
    return true;
}

bool DBFriendList::Exists(const AB::Entities::FriendList& fl)
{
    // Do nothing
    if (fl.uuid.empty() || uuids::uuid(fl.uuid).nil())
    {
        LOG_ERROR << "UUID is empty" << std::endl;
        return false;
    }
    return true;
}

}
