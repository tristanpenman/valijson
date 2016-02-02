/**
 * @file
 *
 * @brief   Adapter implementation for the nlohmann json parser library.
 *
 * Include this file in your program to enable support for nlohmann json.
 *
 * This file defines the following classes (not in this order):
 *  - NlohmannJsonAdapter
 *  - NlohmannJsonArray
 *  - NlohmannJsonValueIterator
 *  - NlohmannJsonFrozenValue
 *  - NlohmannJsonObject
 *  - NlohmannJsonObjectMember
 *  - NlohmannJsonObjectMemberIterator
 *  - NlohmannJsonValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is NlohmannJsonAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#pragma once
#ifndef __VALIJSON_ADAPTERS_NLOHMANN_JSON_ADAPTER_HPP
#define __VALIJSON_ADAPTERS_NLOHMANN_JSON_ADAPTER_HPP

#endif