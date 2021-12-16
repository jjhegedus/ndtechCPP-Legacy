#pragma once

#include "pch.h"

#include <map>
#include <vector>
#include <tuple>
#include <boost/fiber/all.hpp>

namespace ndtech {

  template <typename ItemType, typename NameType = std::string>
  struct NamedItemStore {

    std::map<NameType, ItemType> m_items;
    boost::fibers::mutex m_itemsMutex;
    boost::fibers::condition_variable m_itemsCV;

    std::vector<std::tuple<NameType, ItemType, std::string>> m_itemActions;
    boost::fibers::mutex m_itemActionsMutex;

    bool m_running = true;



    void RunOn(boost::fibers::fiber& fiber) {
      fiber = boost::fibers::fiber(&NamedItemStore::Run, this);
    }

    void Run() {
      while (m_running) {
        {
          std::lock_guard<boost::fibers::mutex> lock(m_itemActionsMutex);
          for (auto itemAction : m_itemActions) {
            if (std::get<2>(itemAction) == "a") {
              std::lock_guard<boost::fibers::mutex> itemsLock(m_itemsMutex);
              m_items.insert_or_assign(std::get<0>(itemAction), std::get<1>(itemAction));
              m_itemsCV.notify_all();
              //std::cout << "NamedItemStore threadId = " << std::this_thread::get_id() << ": added item itemName = " << std::get<0>(itemAction) << " itemValue = " << std::get<1>(itemAction) << std::endl;
            }
            else if (std::get<2>(itemAction) == "r") {
              std::lock_guard<boost::fibers::mutex> itemsLock(m_itemsMutex);
              auto foundItem = m_items.find(std::get<0>(itemAction));
              if (foundItem != m_items.end()) {
                m_items.erase(foundItem);
                m_itemsCV.notify_all();
                //std::cout << "NamedItemStore threadId = " << std::this_thread::get_id() << ": removed item itemName = " << std::get<0>(itemAction) << std::endl;
              }
              else {
                //std::cout << "NamedItemStore threadId = " << std::this_thread::get_id() << ": unable to remove item itemName = " << std::get<0>(itemAction) << " because it does not exist." << std::endl;
              }
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

    void AddItem(NameType key, ItemType value) {
      std::lock_guard<boost::fibers::mutex> lock(m_itemActionsMutex);
      m_itemActions.push_back(std::make_tuple(key, value, "a"));
      m_itemsCV.notify_all();
    }

    void RemoveItem(NameType itemName) {
      std::lock_guard<boost::fibers::mutex> lock(m_itemActionsMutex);
      m_itemActions.push_back(std::make_tuple(itemName, "", "r"));
    }

    ItemType GetItem(NameType itemName) {
      std::unique_lock<boost::fibers::mutex> lock(m_itemsMutex);
      while (m_items.find(itemName) == m_items.end()) {
        m_itemsCV.wait(lock);
      }
      return (*(m_items.find(itemName))).second;
    }

    bool ItemExists(NameType itemName) {

      std::unique_lock<boost::fibers::mutex> lock(m_itemsMutex);
      if (m_items.find(itemName) != m_items.end()) {
        return true;
      }
      return false;

    }

  };

}