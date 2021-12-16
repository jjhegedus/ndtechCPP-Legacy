#pragma once

#include "pch.h"

#include <map>
#include <vector>
#include <tuple>
#include <boost/fiber/all.hpp>
#include <ndtech/TypeUtilities.h>
#include <iostream>
#include <utility>

namespace ndtech {

  template <typename IdType, typename... ItemType>
  struct MultiItemStore {

    using ItemIndexSequence = std::index_sequence_for<ItemType...>;

    std::tuple<std::vector<IdType>, std::vector<ItemType>...> m_items;
    boost::fibers::mutex m_itemsMutex;
    boost::fibers::condition_variable m_itemsCV;

    std::vector<std::tuple<std::string, IdType, ItemType...>> m_itemActions;
    boost::fibers::mutex m_itemActionsMutex;

    bool m_running = true;

    MultiItemStore() {

    }

    MultiItemStore(const MultiItemStore<IdType, ItemType...>& other) {
      this->m_items = other.m_items;
      this->m_itemActions = other.m_itemActions;
    }

    MultiItemStore& operator=(const MultiItemStore<IdType, ItemType...>& rhs) {
      MultiItemStore lhs;
      lhs.m_items = rhs.m_items;
      lhs.m_itemActions = rhs.m_itemActions;

      return lhs;
    }


    void RunOn(boost::fibers::fiber& fiber) {
      fiber = boost::fibers::fiber(&MultiItemStore::Run, this);
    }

    template<size_t Index>
    void AddTupleItemToVectorFromTupleByIndex(std::tuple<ItemType...> tuple, std::tuple<std::vector<ItemType>...>& tupleOfItemTypeVectors) {
      std::get<Index>(tupleOfItemTypeVectors).push_back(std::get<Index>(tuple));
    }

    template <size_t... Index>
    void AddTupleToVectorsByIndex(std::tuple<IdType, ItemType...> tuple, std::tuple<std::vector<IdType>, std::vector<ItemType>...>& tupleOfItemTypeVectors, std::index_sequence<Index...>) {
      (AddTupleItemToVectorFromTupleByIndex<Index>(tuple, tupleOfItemTypeVectors), ...);
    }

    void AddTupleToVectors(std::tuple<IdType, ItemType...> tuple, std::tuple<std::vector<IdType>, std::vector<ItemType>...>& tupleOfItemTypeVectors) {

      using IndexSequenceForItemType = std::index_sequence_for<IdType, ItemType...>;
      AddTupleToVectorsByIndex(tuple, tupleOfItemTypeVectors, IndexSequenceForItemType{});
    }

    template<size_t Index>
    void UpdateTupleItemToVectorFromTupleByIndex(std::tuple<ItemType...> tuple, std::tuple<std::vector<ItemType>...>& tupleOfItemTypeVectors, size_t idIndex) {
      std::get<Index>(tupleOfItemTypeVectors).at(idIndex) = std::get<Index>(tuple);
    }

    template <size_t... Index>
    void UpdateTupleToVectorsByIndex(std::tuple<IdType, ItemType...> tuple, std::tuple<std::vector<IdType>, std::vector<ItemType>...>& tupleOfItemTypeVectors, std::index_sequence<Index...>, size_t idIndex) {
      (UpdateTupleItemToVectorFromTupleByIndex<Index>(tuple, tupleOfItemTypeVectors, idIndex), ...);
    }

    void UpdateTupleToVectors(std::tuple<IdType, ItemType...> tuple, std::tuple<std::vector<IdType>, std::vector<ItemType>...>& tupleOfItemTypeVectors, size_t idIndex) {

      using IndexSequenceForItemType = std::index_sequence_for<IdType, ItemType...>;
      UpdateTupleToVectorsByIndex(tuple, tupleOfItemTypeVectors, IndexSequenceForItemType{}, idIndex);
    }

    template<size_t Index>
    void RemoveFromTupleOfVectorsByIndexImpl(size_t idIndex, std::tuple<std::vector<ItemType>...>& tupleOfItemTypeVectors) {
      std::get<Index>(tupleOfItemTypeVectors).erase(std::get<Index>(tupleOfItemTypeVectors).begin() + idIndex);
    }

    template <size_t... Index>
    void RemoveFromTupleOfVectorsByIndex(size_t idIndex, std::tuple<std::vector<IdType>, std::vector<ItemType>...>& tupleOfItemTypeVectors, std::index_sequence<Index...>) {
      (RemoveFromTupleOfVectorsByIndexImpl<Index>(idIndex, tupleOfItemTypeVectors), ...);
    }

    void RemoveFromTupleOfVectors(size_t idIndex, std::tuple<std::vector<IdType>, std::vector<ItemType>...>& tupleOfItemTypeVectors) {

      using IndexSequenceForItemType = std::index_sequence_for<IdType, ItemType...>;
      RemoveFromTupleOfVectorsByIndex(idIndex, tupleOfItemTypeVectors, IndexSequenceForItemType{});
    }


    //template<size_t Index, typename... ItemType>
    //std::tuple<IdType, ItemType...>  MakeTupleFromVectorsUsingIndexImpl(size_t idIndex, std::tuple<std::vector<IdType>, std::vector<ItemType>...>& tupleOfItemTypeVectors) {
    //  auto retVal = std::make_tuple(std::get<Index>(tupleOfItemTypeVectors)[idIndex]);
    //  return retVal;
    //}

    template <size_t... Index>
    std::tuple<IdType, ItemType...>  MakeTupleFromVectorsUsingIndex(size_t idIndex, std::tuple<std::vector<IdType>, std::vector<ItemType>...>& tupleOfItemTypeVectors, std::index_sequence<Index...>) {
      //auto retVal = (MakeTupleFromVectorsUsingIndexImpl<Index>(idIndex, tupleOfItemTypeVectors), ...);
      auto retVal = std::make_tuple(std::get<Index>(tupleOfItemTypeVectors)[idIndex]...);
      return retVal;
    }

    std::tuple<IdType, ItemType...> MakeTupleFromVectors(size_t idIndex, std::tuple<std::vector<IdType>, std::vector<ItemType>...>& tupleOfItemTypeVectors) {
      using IndexSequenceForItemType = std::index_sequence_for<IdType, ItemType...>;
      return MakeTupleFromVectorsUsingIndex(idIndex, tupleOfItemTypeVectors, IndexSequenceForItemType{});
    }

    void Run() {
      while (m_running) {
        {
          std::lock_guard<boost::fibers::mutex> lock(m_itemActionsMutex);
          for (auto itemAction : m_itemActions) {
            std::string transactionType = std::get<0>(itemAction);
            IdType itemActionId = std::get<1>(itemAction);

            if (transactionType == "am") {
              std::lock_guard<boost::fibers::mutex> itemsLock(m_itemsMutex);

              // Get the item Ids
              auto ids = std::get<0>(m_items);

              // Then check if the id of the itemActionId is in ids
              auto foundId = std::find_if(
                ids.begin(),
                ids.end(),
                [itemActionId](auto testId) {
                  return itemActionId == testId;
                });

              auto tupleWithoutTransactionType = ndtech::TypeUtilities::tail<1>(itemAction);

              if (foundId == ids.end()) {
                // Add the new item to the item actions
                AddTupleToVectors(tupleWithoutTransactionType, m_items);
              }
              else {
                // Update the existing records

                // First get the index of the record we found
                size_t index = std::distance(ids.begin(), foundId);

                // Now update all of the items based on the passed in tuple
                UpdateTupleToVectors(tupleWithoutTransactionType, m_items, index);
              }

              m_itemsCV.notify_all();
            }

            else if (transactionType == "rm") {
              std::lock_guard<boost::fibers::mutex> itemsLock(m_itemsMutex);

              // Get the item Ids
              auto ids = std::get<0>(m_items);

              // Then check if the id of the itemActionId is in ids
              auto foundId = std::find_if(
                ids.begin(),
                ids.end(),
                [itemActionId](auto testId) {
                  return itemActionId == testId;
                });

              if (foundId != ids.end()) {
                // First get the index of the record we found
                size_t index = std::distance(ids.begin(), foundId);

                // Remove the item from m_items
                RemoveFromTupleOfVectors(index, m_items);
              }

              m_itemsCV.notify_all();
            }
          }

          m_itemActions.clear();

        }

        boost::this_fiber::yield();
      }
    }

    void Stop() {
      m_running = false;
    }

    void AddItem(IdType key, ItemType... value) {
      std::lock_guard<boost::fibers::mutex> lock(m_itemActionsMutex);
      m_itemActions.push_back(std::make_tuple("am", key, value...));
      m_itemsCV.notify_all();
    }

    void RemoveItem(IdType itemId) {
      std::lock_guard<boost::fibers::mutex> lock(m_itemActionsMutex);
      m_itemActions.push_back(std::make_tuple("rm", itemId, ItemType{}...));
    }

    std::tuple<ItemType...> GetItem(IdType itemId) {
      std::unique_lock<boost::fibers::mutex> lock(m_itemsMutex);
      auto keys = std::get<0>(m_items);
      auto iter = std::find(keys.begin(), keys.end(), itemId);
      while (iter == keys.end()) {
        m_itemsCV.wait(lock);
      }

      size_t index = std::distance(keys.begin(), iter);
      return ndtech::TypeUtilities::tail<1>(MakeTupleFromVectors(index, m_items));
    }

    bool ItemExists(IdType itemName) {

      std::unique_lock<boost::fibers::mutex> lock(m_itemsMutex);
      if (m_items.find(itemName) != m_items.end()) {
        return true;
      }
      return false;

    }

  };

}