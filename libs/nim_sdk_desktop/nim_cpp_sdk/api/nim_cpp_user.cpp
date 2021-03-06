﻿/** @file nim_cpp_user.cpp
  * @brief NIM SDK提供的用户相关接口
  * @copyright (c) 2015-2016, NetEase Inc. All rights reserved
  * @author towik, Oleg
  * @date 2015/8/17
  */

#include "nim_cpp_user.h"
#include "nim_sdk_util.h"
#include "nim_json_util.h"
#include "nim_cpp_win32_demo_helper.h"
#include "nim_string_util.h"
#include "nim_string_util.h"

namespace nim
{

typedef	void (*nim_user_reg_special_relationship_changed_cb)(const char *json_extension, nim_user_special_relationship_change_cb_func cb, const void *user_data);
typedef	void (*nim_user_set_black)(const char *accid, bool set_black, const char *json_extension, nim_user_opt_cb_func cb, const void *user_data);
typedef	void (*nim_user_set_mute)(const char *accid, bool set_mute, const char *json_extension, nim_user_opt_cb_func cb, const void *user_data);
typedef	void (*nim_user_get_mute_blacklist)(const char *json_extension, nim_user_sync_muteandblacklist_cb_func cb, const void *user_data);

typedef void (*nim_user_reg_user_name_card_changed_cb)(const char *json_extension, nim_user_name_card_change_cb_func cb, const void *user_data);
typedef void (*nim_user_get_user_name_card)(const char *accids, const char *json_extension, nim_user_get_user_name_card_cb_func cb, const void *user_data);
typedef void (*nim_user_get_user_name_card_online)(const char *accids, const char *json_extension, nim_user_get_user_name_card_cb_func cb, const void *user_data);
typedef void (*nim_user_update_my_user_name_card)(const char *info_json, const char *json_extension, nim_user_update_my_name_card_cb_func cb, const void *user_data);

static void CallbackSetRelation(int res_code, const char *accid, bool opt, const char *json_extension, const void *callback)
{
	if (callback)
	{
		User::SetRelationCallback* cb_pointer = (User::SetRelationCallback*)callback;
		if (*cb_pointer)
		{
			PostTaskToUIThread(std::bind((*cb_pointer), (NIMResCode)res_code, PCharToString(accid), opt));
			//(*cb_pointer)((NIMResCode)res_code, PCharToString(accid), opt);
		}
		delete cb_pointer;
	}
}

static void CallbackGetBlackList(int res_code, const char *mute_black_list_json, const char *json_extension, const void *callback)
{
	if (callback)
	{
		User::GetBlackListCallback* cb_pointer = (User::GetBlackListCallback*)callback;
		if (*cb_pointer)
		{
			std::list<BlackListInfo> black_list;
			std::list<MuteListInfo> mute_list;
			ParseSpecialListInfo(PCharToString(mute_black_list_json), black_list, mute_list);
			PostTaskToUIThread(std::bind((*cb_pointer), (NIMResCode)res_code, black_list));
			//(*cb_pointer)((NIMResCode)res_code, black_list);
		}
		delete cb_pointer;
	}
}

static void CallbackGetMuteList(int res_code, const char *mute_black_list_json, const char *json_extension, const void *callback)
{
	if (callback)
	{
		User::GetMuteListCallback* cb_pointer = (User::GetMuteListCallback*)callback;
		if (*cb_pointer)
		{
			std::list<BlackListInfo> black_list;
			std::list<MuteListInfo> mute_list;
			ParseSpecialListInfo(PCharToString(mute_black_list_json), black_list, mute_list);
			PostTaskToUIThread(std::bind((*cb_pointer), (NIMResCode)res_code, mute_list));
			//(*cb_pointer)((NIMResCode)res_code, mute_list);
		}
		delete cb_pointer;
	}
}

static void CallbackGetUserNameCard(const char *result_json, const char *json_extension, const void *callback)
{
	if (callback)
	{
		User::GetUserNameCardCallback* cb_pointer = (User::GetUserNameCardCallback*)callback;
		if (*cb_pointer)
		{
			std::list<UserNameCard> users_info;
			ParseNameCards(PCharToString(result_json), users_info);
			PostTaskToUIThread(std::bind((*cb_pointer), users_info));
			//(*cb_pointer)(users_info);
		}
		delete cb_pointer;
	}
}

static void CallbackUpdateMyUserNameCard(int res_code, const char *json_extension, const void *callback)
{
	if (callback)
	{
		User::UpdateMyUserNameCardCallback* cb_pointer = (User::UpdateMyUserNameCardCallback*)callback;
		if (*cb_pointer)
		{
			PostTaskToUIThread(std::bind((*cb_pointer), (NIMResCode)res_code));
			//(*cb_pointer)((NIMResCode)res_code);
		}
		delete cb_pointer;
	}
}

static void CallbackSpecialRelationChange(NIMUserSpecialRelationshipChangeType type, const char *result_json, const char *json_extension, const void *callback)
{
	if (callback)
	{
		User::SpecialRelationshipChangedCallback* cb_pointer = (User::SpecialRelationshipChangedCallback*)callback;
		if (*cb_pointer)
		{
			SpecialRelationshipChangeEvent change_event;
			change_event.type_ = type;
			change_event.content_ = PCharToString(result_json);
			PostTaskToUIThread(std::bind((*cb_pointer), change_event));
			//(*cb_pointer)(change_event);
		}
	}
}

static void CallbackUserNameCardChange(const char *result_json, const char *json_extension, const void *callback)
{
	if (callback)
	{
		User::UserNameCardChangedCallback* cb_pointer = (User::UserNameCardChangedCallback*)callback;
		if (*cb_pointer)
		{
			std::list<UserNameCard> users_info;
			ParseNameCards(PCharToString(result_json), users_info);
			PostTaskToUIThread(std::bind((*cb_pointer), users_info));
			//(*cb_pointer)(users_info);
		}
	}
}

static User::SpecialRelationshipChangedCallback* g_cb_relation_changed_ = nullptr;
void User::RegSpecialRelationshipChangedCb(const SpecialRelationshipChangedCallback& cb, const std::string& json_extension)
{
	if (g_cb_relation_changed_)
	{
		delete g_cb_relation_changed_;
		g_cb_relation_changed_ = nullptr;
	}
	g_cb_relation_changed_ = new SpecialRelationshipChangedCallback(cb);
	return NIM_SDK_GET_FUNC(nim_user_reg_special_relationship_changed_cb)(json_extension.c_str(), &CallbackSpecialRelationChange, g_cb_relation_changed_);
}

bool User::SetBlack(const std::string& accid, bool set_black, const SetBlackCallback& cb, const std::string& json_extension)
{
	if (accid.empty())
		return false;

	SetBlackCallback* cb_pointer = nullptr;
	if (cb)
	{
		cb_pointer = new SetBlackCallback(cb);
	}
	NIM_SDK_GET_FUNC(nim_user_set_black)(accid.c_str(), set_black, json_extension.c_str(), &CallbackSetRelation, cb_pointer);

	return true;
}

bool User::SetMute(const std::string& accid, bool set_mute, const SetMuteCallback& cb, const std::string& json_extension)
{
	if (accid.empty())
		return false;

	SetMuteCallback* cb_pointer = nullptr;
	if (cb)
	{
		cb_pointer = new SetMuteCallback(cb);
	}
	NIM_SDK_GET_FUNC(nim_user_set_mute)(accid.c_str(), set_mute, json_extension.c_str(), &CallbackSetRelation, cb_pointer);

	return true;
}

void User::GetMutelist(const GetMuteListCallback& cb, const std::string& json_extension)
{
	GetMuteListCallback* cb_pointer = nullptr;
	if (cb)
	{
		cb_pointer = new GetMuteListCallback(cb);
	}
	return NIM_SDK_GET_FUNC(nim_user_get_mute_blacklist)(json_extension.c_str(), &CallbackGetMuteList, cb_pointer);
}

void User::GetBlacklist(const GetBlackListCallback& cb, const std::string& json_extension)
{
	GetBlackListCallback* cb_pointer = nullptr;
	if (cb)
	{
		cb_pointer = new GetBlackListCallback(cb);
	}
	return NIM_SDK_GET_FUNC(nim_user_get_mute_blacklist)(json_extension.c_str(), &CallbackGetBlackList, cb_pointer);
}

static User::UserNameCardChangedCallback* g_cb_uinfo_changed_ = nullptr;
void User::RegUserNameCardChangedCb(const UserNameCardChangedCallback & cb, const std::string & json_extension)
{
	if (g_cb_uinfo_changed_)
	{
		delete g_cb_uinfo_changed_;
		g_cb_uinfo_changed_ = nullptr;
	}
	g_cb_uinfo_changed_ = new UserNameCardChangedCallback(cb);
	return NIM_SDK_GET_FUNC(nim_user_reg_user_name_card_changed_cb)(json_extension.c_str(), &CallbackUserNameCardChange, g_cb_uinfo_changed_);
}

bool User::GetUserNameCard(const std::list<std::string>& accids, const GetUserNameCardCallback& cb, const std::string& json_extension /*= ""*/)
{
	if (accids.empty())
		return false;

	GetUserNameCardCallback* cb_pointer = nullptr;
	if (cb)
	{
		cb_pointer = new GetUserNameCardCallback(cb);
	}

	Json::Value values;
	for (auto iter = accids.cbegin(); iter != accids.cend(); ++iter)
		values.append(*iter);
	
	NIM_SDK_GET_FUNC(nim_user_get_user_name_card)(GetJsonStringWithNoStyled(values).c_str(), json_extension.c_str(), &CallbackGetUserNameCard, cb_pointer);

	return true;
}

bool User::GetUserNameCardOnline(const std::list<std::string>& accids, const GetUserNameCardCallback& cb, const std::string& json_extension /*= ""*/)
{
	if (accids.empty())
		return false;

	GetUserNameCardCallback* cb_pointer = nullptr;
	if (cb)
	{
		cb_pointer = new GetUserNameCardCallback(cb);
	}

	Json::Value values;
	for (auto iter = accids.cbegin(); iter != accids.cend(); ++iter)
		values.append(*iter);

	NIM_SDK_GET_FUNC(nim_user_get_user_name_card_online)(GetJsonStringWithNoStyled(values).c_str(), json_extension.c_str(), &CallbackGetUserNameCard, cb_pointer);

	return true;
}

bool User::UpdateMyUserNameCard(const UserNameCard& namecard, const UpdateMyUserNameCardCallback& cb, const std::string& json_extension /*= ""*/)
{
	if (namecard.GetAccId().empty())
		return false;

	UpdateMyUserNameCardCallback* cb_pointer = nullptr;
	if (cb)
	{
		cb_pointer = new UpdateMyUserNameCardCallback(cb);
	}
	NIM_SDK_GET_FUNC(nim_user_update_my_user_name_card)(namecard.ToJsonString().c_str(), json_extension.c_str(), &CallbackUpdateMyUserNameCard, cb_pointer);

	return true;
}

bool User::ParseBlackListInfoChange(const SpecialRelationshipChangeEvent& change_event, BlackListInfo& info)
{
	if (change_event.type_ != kNIMUserSpecialRelationshipChangeTypeMarkBlack)
		return false;

	Json::Value values;
	Json::Reader reader;
	if (reader.parse(change_event.content_, values) && values.isObject())
	{
		info.accid_ = values["accid"].asString();
		info.set_black_ = values["black"].asBool();
		return true;
	}

	return false;
}

bool User::ParseMuteListInfoChange(const SpecialRelationshipChangeEvent& change_event, MuteListInfo& info)
{
	if (change_event.type_ != kNIMUserSpecialRelationshipChangeTypeMarkMute)
		return false;

	Json::Value values;
	Json::Reader reader;
	if (reader.parse(change_event.content_, values) && values.isObject())
	{
		info.accid_ = values["accid"].asString();
		info.set_mute_ = values["mute"].asBool();
		return true;
	}

	return false;
}

bool User::ParseSyncSpecialRelationshipChange(const SpecialRelationshipChangeEvent& change_event, std::list<BlackListInfo>& black_list, std::list<MuteListInfo>& mute_list)
{
	if (change_event.type_ != kNIMUserSpecialRelationshipChangeTypeSyncMuteAndBlackList)
		return false;

	return ParseSpecialListInfo(change_event.content_, black_list, mute_list);
}

void User::UnregUserCb()
{
	if (g_cb_relation_changed_)
	{
		delete g_cb_relation_changed_;
		g_cb_relation_changed_ = nullptr;
	}

	if (g_cb_uinfo_changed_)
	{
		delete g_cb_uinfo_changed_;
		g_cb_uinfo_changed_ = nullptr;
	}
}
}
